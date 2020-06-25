#include "alarm_controller.h"
#include "main.h"

#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

#define UPDATE_PERIOD_MS 10
#define BLINK_NUMBER 3

#define LIGHT_INTENSITY 255
#define MOVEMENT_SENSIBILITY 20

module_t *app;
volatile control_mode_t control_mode;
uint8_t blink_state = 0;

// Imu report struct
typedef struct __attribute__((__packed__))
{
    union {
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

//*************** Standard functions****************

void rx_alarm_controller_cb(module_t *module, msg_t *msg)
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

void alarm_controller_init(void)
{
    // By default this app running
    control_mode.mode_control = PLAY;
    // Create App
    app = luos_module_create(rx_alarm_controller_cb, ALARM_CONTROLLER_APP, "alarm_control", STRINGIFY(VERSION));
}

void alarm_controller_loop(void)
{
    static short previous_id = -1;
    static uint8_t blink = 0;
    static uint8_t blink_nb = BLINK_NUMBER * 2;
    static uint32_t last_blink = 0;

    // ********** hot plug management ************
    // Check if we have done the first init or if module Id have changed
    if (previous_id != id_from_module(app))
    {
        if (id_from_module(app) == 0)
        {
            // We don't have any ID, meaning no detection occure or detection is occuring.
            if (previous_id == -1)
            {
                // This is the really first init, we have to make it.
                // Be sure the network is powered up 1500 ms before starting a detection
                if (HAL_GetTick() > 1500)
                {
                    // No detection occure, do it
                    detect_modules(app);
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
            int id = id_from_type(COLOR_MOD);
            if (id > 0)
            {
                msg_t msg;
                msg.header.target = id;
                msg.header.target_mode = IDACK;
                time_luos_t time = time_from_sec(0.5f);
                time_to_msg(&time, &msg);
                luos_send(app, &msg);
            }
            // try to find a gps and set parameters to disable quaternion and send back Gyro acceleration and euler.
            imu_report_t report;
            report.gyro = 1;
            report.euler = 1;
            report.quat = 0;
            id = id_from_type(IMU_MOD);
            if (id > 0)
            {
                msg_t msg;
                msg.header.cmd = PARAMETERS;
                msg.header.size = sizeof(imu_report_t);
                msg.header.target = id;
                msg.header.target_mode = IDACK;
                memcpy(msg.data, &report, sizeof(imu_report_t));
                luos_send(app, &msg);

                // Setup auto update each UPDATE_PERIOD_MS on gps
                // This value is resetted on all module at each detection
                // It's important to setting it each time.
                time_luos_t time = time_from_ms(UPDATE_PERIOD_MS);
                time_to_msg(&time, &msg);
                msg.header.cmd = UPDATE_PUB;
                luos_send(app, &msg);
            }
            previous_id = id_from_module(app);
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
            int id = id_from_alias("buzzer_mod");
            if (id > 0)
            {
                msg_t msg;
                msg.header.target = id;
                msg.header.target_mode = IDACK;
                msg.header.cmd = IO_STATE;
                msg.header.size = 1;
                msg.data[0] = 0;
                luos_send(app, &msg);
            }
        }
        if (blink_nb < (BLINK_NUMBER * 2))
        {
            if ((HAL_GetTick() - last_blink) >= 500)
            {
                blink_nb++;
                int id = id_from_type(COLOR_MOD);
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
                    color_to_msg(&color, &msg);
                    luos_send(app, &msg);
                }
                id = id_from_alias("horn");
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
                    luos_send(app, &msg);
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
