#include "alarm_controller.h"

#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

#define UPDATE_PERIOD_MS 10
#define BLINK_NUMBER 6

module_t *app;
volatile uint8_t lock = 1;
uint8_t blink_state = 0;
volatile uint8_t setup = 0;
volatile uint32_t last_systick = 0;
uint8_t init = 0;

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
    START_CONTROLLER_APP = LUOS_LAST_MOD,
    ALARM_CONTROLLER_APP
} alarm_t;

//*************** Local functions****************

void get_gps()
{
    int id = id_from_alias("gps");
    if (id > 0)
    {
        // we get a lock, ask it state
        msg_t msg;
        msg.header.cmd = ASK_PUB_CMD;
        msg.header.size = 0;
        msg.header.target = id;
        msg.header.target_mode = IDACK;
        luos_send(app, &msg);
    }
}

//*************** Standard functions****************

void rx_alarm_controller_cb(module_t *module, msg_t *msg)
{
    if (msg->header.cmd == GYRO_3D)
    {
        // this is the gps reply
        if (lock)
        {
            float value[3];
            memcpy(value, msg->data, msg->header.size);
            if ((value[0] > 300) || (value[1] > 300) || (value[2] > 300))
            {
                blink_state = 1;
            }
        }
        return;
    }
    if (msg->header.cmd == QUATERNION)
    {
        // setup gps
        setup = 1;
    }
    if (msg->header.cmd == IO_STATE)
    {
        // this is the lock info
        lock = msg->data[0];
        return;
    }
}

void alarm_controller_init(void)
{
    app = luos_module_create(rx_alarm_controller_cb, ALARM_CONTROLLER_APP, "alarm_control", STRINGIFY(VERSION));
    last_systick = HAL_GetTick();
}

void alarm_controller_loop(void)
{
    static uint8_t blink = 0;
    static uint8_t blink_nb = BLINK_NUMBER;
    static uint32_t last_blink = 0;

    if (!init)
    {
        if (id_from_alias("alarm_control") == -1)
        {
            if (HAL_GetTick() - last_systick > 1500)
            {
                // No detection occure, do it
                detect_modules(app);
            }
        }
        else
        {
            // try to find an alarm and set light transition time
            int id = id_from_alias("alarm");
            float time = 0.5f;
            if (id > 0)
            {
                msg_t msg;
                msg.header.cmd = TIME;
                msg.header.size = sizeof(float);
                msg.header.target = id;
                msg.header.target_mode = IDACK;
                memcpy(msg.data, &time, sizeof(float));
                luos_send(app, &msg);
                init = 1;
            }
            // try to find a gps and set parameters to send back Gyro acceleration
            id = id_from_alias("gps");
            imu_report_t report;
            report.gyro = 1;
            report.euler = 1;
            report.quat = 0;
            if (id > 0)
            {
                msg_t msg;
                msg.header.cmd = PARAMETERS;
                msg.header.size = sizeof(imu_report_t);
                msg.header.target = id;
                msg.header.target_mode = IDACK;
                memcpy(msg.data, &report, sizeof(imu_report_t));
                luos_send(app, &msg);
            }
            init = 1;
        }
        return;
    }
    if (setup)
    {
        // try to find a gps and set parameters to send back Gyro acceleration
        int id = id_from_alias("gps");
        imu_report_t report;
        report.gyro = 1;
        report.euler = 1;
        report.quat = 0;
        if (id > 0)
        {
            msg_t msg;
            msg.header.cmd = PARAMETERS;
            msg.header.size = sizeof(imu_report_t);
            msg.header.target = id;
            msg.header.target_mode = IDACK;
            memcpy(msg.data, &report, sizeof(imu_report_t));
            luos_send(app, &msg);
        }
        setup = 0;
    }
    // update frequency
    if ((HAL_GetTick() - last_systick) >= UPDATE_PERIOD_MS)
    {
        get_gps();
        last_systick = HAL_GetTick();
    }
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
    if (blink_nb < BLINK_NUMBER)
    {
        if ((HAL_GetTick() - last_blink) >= 500)
        {
            blink_nb++;
            int id = id_from_alias("alarm");
            if (id > 0)
            {
                // we get an alarm, we can set its color
                color_t color;
                if (blink)
                {
                    // bike locked turn led off
                    color.r = 0;
                    color.g = 0;
                    color.b = 0;
                }
                else
                {
                    // bike unlocked turn led green
                    color.r = 30;
                    color.g = 0;
                    color.b = 0;
                }
                msg_t msg;
                msg.header.target = id;
                msg.header.target_mode = IDACK;
                color_to_msg(&color, &msg);
                luos_send(app, &msg);
                blink = (!blink);
                last_blink = HAL_GetTick();
            }
        }
    }
}
