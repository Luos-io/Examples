#include "start_controller.h"
#include "main.h"

#define LIGHT_INTENSITY 255

#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

#define UPDATE_PERIOD_MS 10

module_t *app;
volatile control_mode_t control_mode;
uint8_t lock = 1;
uint8_t last_btn_state = 0;
uint8_t state_switch = 0;
uint8_t init = 0;

typedef enum
{
    ALARM_CONTROLLER_APP = LUOS_LAST_TYPE,
    START_CONTROLLER_APP
} alarm_t;

void rx_start_controller_cb(module_t *module, msg_t *msg)
{
    if (msg->header.cmd == IO_STATE)
    {
        if (control_mode.mode_control == PLAY)
        {
            if (type_from_id(msg->header.source) == STATE_MOD)
            {
                // this is the button reply we have filter it to manage monostability
                if ((!last_btn_state) & (last_btn_state != msg->data[0]))
                {
                    lock = (!lock);
                    state_switch++;
                }
            }
            else
            {
                // this is an already filtered information
                if ((lock != msg->data[0]))
                {
                    lock = msg->data[0];
                    state_switch++;
                }
            }
            last_btn_state = msg->data[0];
        }
        return;
    }
    if (msg->header.cmd == CONTROL)
    {
        control_mode.unmap = msg->data[0];
        return;
    }
}

void start_controller_init(void)
{
    // By default this app running
    control_mode.mode_control = PLAY;
    // Create App
    app = luos_module_create(rx_start_controller_cb, START_CONTROLLER_APP, "start_control", STRINGIFY(VERSION));
}

void start_controller_loop(void)
{
    static short previous_id = -1;
    static uint32_t switch_date = 0;
    static uint8_t animation_state = 0;
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
                // Be sure the network is powered up 1000 ms before starting a detection
                if (HAL_GetTick() > 1000)
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
            int id = id_from_alias("lock");
            if (id > 0)
            {
                msg_t msg;
                msg.header.target = id;
                msg.header.target_mode = IDACK;
                // Setup auto update each UPDATE_PERIOD_MS on button
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
    // ********** non blocking button management ************
    if (state_switch & (control_mode.mode_control == PLAY) & (animation_state == 0))
    {
        msg_t msg;
        msg.header.target_mode = IDACK;
        // Share the lock state with the alarm_control app
        int id = id_from_alias("alarm_control");
        if (id > 0)
        {
            // we have an alarm_controller App control it
            control_mode_t alarm_control;
            if (lock)
            {
                // Bike is locked, alarm need to run.
                alarm_control.mode_control = PLAY;
            }
            else
            {
                // Bike is unlocked alarm should be sutted down.
                alarm_control.mode_control = STOP;
            }
            // send message
            msg.header.target = id;
            msg.header.cmd = CONTROL;
            msg.header.size = sizeof(control_mode_t);
            msg.data[0] = alarm_control.unmap;
            luos_send(app, &msg);
        }
        // The button state switch, change the led consequently
        state_switch = 0;
        id = id_from_type(COLOR_MOD);
        if (id > 0)
        {
            // we have an alarm, we can set its color
            color_t color;
            color.r = 0;
            color.g = 0;
            color.b = 0;
            if (!lock)
            {
                color.g = LIGHT_INTENSITY;
            }
            else
            {
                color.b = LIGHT_INTENSITY;
            }
            msg.header.target = id;
            color_to_msg(&color, &msg);
            luos_send(app, &msg);
        }
        id = id_from_alias("horn");
        if (id > 0)
        {
            // we get a horn
            msg.header.target = id;
            msg.header.size = sizeof(uint8_t);
            msg.header.cmd = IO_STATE;
            // turn the horn on/off
            msg.data[0] = 1;
            luos_send(app, &msg);
        }
        // try to reach a buzzer and drive it to make a happy sound
        if (!lock)
        {
            id = id_from_alias("buzzer_mod");
            if (id > 0)
            {
                msg.header.target = id;
                msg.header.cmd = IO_STATE;
                msg.header.size = 1;
                msg.data[0] = 1;
                luos_send(app, &msg);
            }
        }
        // Save switch date
        switch_date = HAL_GetTick();
        animation_state++;
    }
    // This part is a start stop animation using available modules
    if (((HAL_GetTick() - switch_date) > 100) & (animation_state == 1))
    {
        // 100ms after button turn of light and horn
        msg_t msg;
        msg.header.target_mode = IDACK;
        int id = id_from_alias("horn");
        if (id > 0)
        {
            // we get a horn
            msg.header.target = id;
            msg.header.size = sizeof(uint8_t);
            msg.header.cmd = IO_STATE;
            // turn the horn on/off
            msg.data[0] = 0;
            luos_send(app, &msg);
        }
        animation_state++;
    }
    if (((HAL_GetTick() - switch_date) > 600) & (animation_state == 2))
    {
        // 600ms after switch turn light depending on the curent lock state
        msg_t msg;
        msg.header.target_mode = IDACK;
        int id = id_from_type(COLOR_MOD);
        if (id > 0)
        {
            // we have an alarm, we can set its color
            color_t color;
            if (lock)
            {
                color.r = 0;
                color.g = 0;
                color.b = 0;
            }
            else
            {
                color.r = LIGHT_INTENSITY;
                color.g = LIGHT_INTENSITY;
                color.b = LIGHT_INTENSITY;
            }
            msg.header.target = id;
            color_to_msg(&color, &msg);
            luos_send(app, &msg);
        }
        animation_state = 0;
    }
}
