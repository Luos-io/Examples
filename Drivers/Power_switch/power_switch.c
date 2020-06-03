#include "main.h"
#include "power_switch.h"

#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

void rx_pow_cb(module_t *module, msg_t *msg)
{
    if (msg->header.cmd == IO_STATE)
    {
        HAL_GPIO_WritePin(GPIOA, SWITCH_Pin, msg->data[0]);
        return;
    }
}

void power_switch_init(void)
{
    luos_module_enable_rt(luos_module_create(rx_pow_cb, STATE_MOD, "switch_mod", STRINGIFY(VERSION)));
}

void power_switch_loop(void)
{
}
