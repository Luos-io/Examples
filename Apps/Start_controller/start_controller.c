#include "start_controller.h"

#define UPDATE_PERIOD_MS 10

module_t* app;
uint8_t lock = 1;
uint8_t last_btn_state = 0;
uint8_t state_switch = 0;
uint8_t init = 0;


typedef enum {
    START_CONTROLLER_APP = LUOS_LAST_MOD,
    ALARM_CONTROLLER_APP
} pony_t;

//*************** Local functions****************

void get_btn() {
    int id = id_from_alias("lock");
    if (id > 0) {
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

void rx_start_controller_cb(module_t *module, msg_t *msg) {
    if (msg->header.cmd == IO_STATE) {
        // this is the lock reply
        if ((!last_btn_state) & (last_btn_state != msg->data[0])) {
            lock = (!lock);
            state_switch++;
        }
        last_btn_state = msg->data[0];
        return;
    }
}

void start_controller_init(void) {
    // Create App
    app = luos_module_create(rx_start_controller_cb, START_CONTROLLER_APP, "start_control");
    // Wait a little to be sure everyone have booted
    HAL_Delay(1000);
    // detect modules in the network
    detect_modules(app);
}

void start_controller_loop(void) {
    static uint32_t last_systick = 0;
    // Alarm have not been configured try to do it.
    if (!init){
        // try to find an alarm and set light transition time
        int id = id_from_alias("alarm");
        float time = 0.5f;
        if (id > 0) {
            msg_t msg;
            msg.header.cmd = TIME;
            msg.header.size = sizeof(float);
            msg.header.target = id;
            msg.header.target_mode = IDACK;
            memcpy(msg.data, &time, sizeof(float));
            luos_send(app, &msg);
            init = 1;
        }
    }
    // update the button state each 10ms
    if ((HAL_GetTick() - last_systick) >= UPDATE_PERIOD_MS) {
        get_btn();
        last_systick = HAL_GetTick();
    }
    // check if the button state switch
    if (state_switch) {
        // The button state switch set the new alarm color
        state_switch = 0;
        int id = id_from_alias("alarm");
        if (id > 0) {
            // we have an alarm, we can set its color
            color_t color;
            if (lock) {
                // bike locked turn led off
                color.r = 0;
                color.g = 0;
                color.b = 0;
            } else {
                // bike unlocked turn led green
                color.r = 0;
                color.g = 30;
                color.b = 0;
            }
            msg_t msg;
            msg.header.target = id;
            msg.header.target_mode = IDACK;
            color_to_msg(&color, &msg);
            luos_send(app, &msg);
        }
        // Share the system state with the alarm_control app
        id = id_from_alias("alarm_control");
        if (id > 0) {
            // we have an alarm_controller App setup the lock state
            msg_t msg;
            msg.header.target = id;
            msg.header.target_mode = IDACK;
            msg.header.cmd = IO_STATE;
            msg.header.size = 1;
            msg.data[0] = lock;
            luos_send(app, &msg);
        }
        // try to reach a buzzer and drive it to make a happy sound
        if (!lock) {
            id = id_from_alias("buzzer_mod");
            if (id > 0) {
                msg_t msg;
                msg.header.target = id;
                msg.header.target_mode = IDACK;
                msg.header.cmd = IO_STATE;
                msg.header.size = 1;
                msg.data[0] = 1;
                luos_send(app, &msg);
            }
        }
    }
}
