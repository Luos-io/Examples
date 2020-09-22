/******************************************************************************
 * @file alarm controler
 * @brief application example an alarm controler
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "alarm_controller.h"
#include "main.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

#define UPDATE_PERIOD_MS 10
#define BLINK_NUMBER 3

#define LIGHT_INTENSITY 255
#define MOVEMENT_SENSIBILITY 20

// Imu report struct
typedef struct __attribute__((__packed__))
{
    union
    {
        struct __attribute__((__packed__))
        {
            unsigned short accell : 1;
            unsigned short gyro : 1;
            unsigned short quat : 1;
            unsigned short compass : 1;
            unsigned short euler : 1;
            unsigned short rot_mat : 1;
            unsigned short pedo : 1;
            unsigned short linear_accel : 1;
            unsigned short gravity_vector : 1;
            unsigned short heading : 1;
        };
        unsigned char unmap[2];
    };
} imu_report_t;

typedef enum
{
    ALARM_CONTROLLER_APP = LUOS_LAST_TYPE,
    START_CONTROLLER_APP
} alarm_apps_type_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/
module_t *app;
volatile control_mode_t control_mode;
uint8_t blink_state = 0;
/*******************************************************************************
 * Function
 ******************************************************************************/
static void AlarmController_MsgHandler(module_t *module, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void AlarmController_Init(void)
{
    // By default this app running
    control_mode.mode_control = PLAY;
    // Create App
    app = Luos_CreateModule(AlarmController_MsgHandler, ALARM_CONTROLLER_APP, "alarm_control", STRINGIFY(VERSION));
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void AlarmController_Loop(void)
{
    static short previous_id = -1;
    static uint8_t blink = 0;
    static uint8_t blink_nb = BLINK_NUMBER * 2;
    static uint32_t last_blink = 0;

    // ********** hot plug management ************
    // Check if we have done the first init or if module Id have changed
    if (previous_id != RouteTB_IDFromModule(app))
    {
        if (RouteTB_IDFromModule(app) == 0)
        {
            // We don't have any ID, meaning no detection occure or detection is occuring.
            if (previous_id == -1)
            {
                // This is the really first init, we have to make it.
                // Be sure the network is powered up 1500 ms before starting a detection
                if (HAL_GetTick() > 1500)
                {
                    // No detection occure, do it
                    RouteTB_DetectModules(app);
                }
            }
            else
            {
                // someone is making a detection, let it finish.
                // reset the init state to be ready to setup module at the end of detection
                previous_id = 0;
            }
        }
        else
        {
            // Make modules configurations
            // try to find a RGB led and set light transition time just to be fancy
            int id = RouteTB_IDFromType(COLOR_MOD);
            if (id > 0)
            {
                msg_t msg;
                msg.header.target = id;
                msg.header.target_mode = IDACK;
                time_luos_t time = TimeOD_TimeFrom_s(0.5f);
                TimeOD_TimeToMsg(&time, &msg);
                Luos_SendMsg(app, &msg);
            }
            // try to find a gps and set parameters to disable quaternion and send back Gyro acceleration and euler.
            imu_report_t report;
            report.gyro = 1;
            report.euler = 1;
            report.quat = 0;
            id = RouteTB_IDFromType(IMU_MOD);
            if (id > 0)
            {
                msg_t msg;
                msg.header.cmd = PARAMETERS;
                msg.header.size = sizeof(imu_report_t);
                msg.header.target = id;
                msg.header.target_mode = IDACK;
                memcpy(msg.data, &report, sizeof(imu_report_t));
                Luos_SendMsg(app, &msg);

                // Setup auto update each UPDATE_PERIOD_MS on gps
                // This value is resetted on all module at each detection
                // It's important to setting it each time.
                time_luos_t time = TimeOD_TimeFrom_ms(UPDATE_PERIOD_MS);
                TimeOD_TimeToMsg(&time, &msg);
                msg.header.cmd = UPDATE_PUB;
                Luos_SendMsg(app, &msg);
            }
            previous_id = RouteTB_IDFromModule(app);
        }
        return;
    }
    // ********** non blocking blink ************
    if (control_mode.mode_control == PLAY)
    {
        if (blink_state)
        {
            blink_state = 0;
            blink_nb = 0;
            blink = 0;
            // try to reach a buzzer and drive it if there is
            int id = RouteTB_IDFromAlias("buzzer_mod");
            if (id > 0)
            {
                msg_t msg;
                msg.header.target = id;
                msg.header.target_mode = IDACK;
                msg.header.cmd = IO_STATE;
                msg.header.size = 1;
                msg.data[0] = 0;
                Luos_SendMsg(app, &msg);
            }
        }
        if (blink_nb < (BLINK_NUMBER * 2))
        {
            if ((HAL_GetTick() - last_blink) >= 500)
            {
                blink_nb++;
                int id = RouteTB_IDFromType(COLOR_MOD);
                if (id > 0)
                {
                    // we get a led alarm, set color
                    color_t color;
                    color.r = 0;
                    color.g = 0;
                    color.b = 0;
                    if (!blink)
                    {
                        // turn led red
                        color.r = LIGHT_INTENSITY;
                    }
                    msg_t msg;
                    msg.header.target = id;
                    msg.header.target_mode = IDACK;
                    IlluminanceOD_ColorToMsg(&color, &msg);
                    Luos_SendMsg(app, &msg);
                }
                id = RouteTB_IDFromAlias("horn");
                if (id > 0)
                {
                    // we get a horn
                    uint8_t horn = 0;
                    if (!blink)
                    {
                        // turn the horn on
                        horn = 1;
                    }
                    msg_t msg;
                    msg.header.target = id;
                    msg.header.target_mode = IDACK;
                    msg.header.size = sizeof(uint8_t);
                    msg.header.cmd = IO_STATE;
                    msg.data[0] = horn;
                    Luos_SendMsg(app, &msg);
                }
                blink = (!blink);
                last_blink = HAL_GetTick();
            }
        }
    }
    else
    {
        if (blink_nb != BLINK_NUMBER * 2)
        {
            // reset alarm state
            blink_nb = BLINK_NUMBER * 2;
            blink_state = 0;
            blink = 0;
        }
    }
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this module
 * @param Module destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void AlarmController_MsgHandler(module_t *module, msg_t *msg)
{
    if (msg->header.cmd == GYRO_3D)
    {
        // this is imu informations
        if (control_mode.mode_control == PLAY)
        {
            float value[3];
            memcpy(value, msg->data, msg->header.size);
            if ((value[0] > MOVEMENT_SENSIBILITY) || (value[1] > MOVEMENT_SENSIBILITY) || (value[2] > MOVEMENT_SENSIBILITY))
            {
                blink_state = 1;
            }
        }
        return;
    }
    if (msg->header.cmd == CONTROL)
    {
        control_mode.unmap = msg->data[0];
        return;
    }
}
