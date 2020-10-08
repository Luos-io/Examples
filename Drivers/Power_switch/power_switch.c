/******************************************************************************
 * @file Power switch
 * @brief driver example a simple Power switch
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "main.h"
#include "power_switch.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
static void PowerSwitch_MsgHandler(container_t *container, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void PowerSwitch_Init(void)
{
    Luos_CreateContainer(PowerSwitch_MsgHandler, STATE_MOD, "switch_mod", STRINGIFY(VERSION));
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void PowerSwitch_Loop(void)
{
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this container
 * @param Container destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void PowerSwitch_MsgHandler(container_t *container, msg_t *msg)
{
    if (msg->header.cmd == IO_STATE)
    {
        HAL_GPIO_WritePin(GPIOA, SWITCH_Pin, msg->data[0]);
        return;
    }
}
