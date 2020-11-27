/******************************************************************************
 * @file button
 * @brief driver example a simple button
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "button.h"
#include "gpio.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifdef REV
revision_t revision = {.unmap = REV};
#endif
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
static void Button_MsgHandler(container_t *container, msg_t *msg);
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Button_Init(void)
{
    Luos_CreateContainer(Button_MsgHandler, STATE_MOD, "button_mod", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Button_Loop(void)
{
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this container
 * @param Container destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Button_MsgHandler(container_t *container, msg_t *msg)
{
    if (msg->header.cmd == ASK_PUB_CMD)
    {
        // fill the message infos
        msg_t pub_msg;
        pub_msg.header.cmd = IO_STATE;
        pub_msg.header.target_mode = ID;
        pub_msg.header.target = msg->header.source;
        pub_msg.header.size = sizeof(char);
        pub_msg.data[0] = HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin);
        Luos_SendMsg(container, &pub_msg);
        return;
    }
}
