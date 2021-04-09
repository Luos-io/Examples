/******************************************************************************
 * @file gpio_dev
 * @brief driver example a simple gpio_dev
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include <gpio_dev.h>
#include "main.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
enum
{
    P1,
    P2,
    P3,
    P4,
    P5,
    P6,
    P7,
    P8,
    P9
};
/*******************************************************************************
 * Variables
 ******************************************************************************/
container_t *pin[9];

/*******************************************************************************
 * Function
 ******************************************************************************/
static void rx_digit_write_cb(container_t *container, msg_t *msg);
static void rx_digit_read_cb(container_t *container, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void GpioDev_Init(void)
{
	revision_t revision = {.unmap = REV};
    // ******************* Analog measurement *******************
    // ************* containers creation *******************
    pin[P1] = Luos_CreateContainer(rx_digit_read_cb, STATE_MOD, "digit_read_P1", revision);
    pin[P7] = Luos_CreateContainer(rx_digit_read_cb, STATE_MOD, "digit_read_P7", revision);
    pin[P8] = Luos_CreateContainer(rx_digit_read_cb, STATE_MOD, "digit_read_P8", revision);
    pin[P9] = Luos_CreateContainer(rx_digit_read_cb, STATE_MOD, "digit_read_P9", revision);
    pin[P5] = Luos_CreateContainer(rx_digit_read_cb, STATE_MOD, "digit_read_P5", revision);
    pin[P6] = Luos_CreateContainer(rx_digit_read_cb, STATE_MOD, "digit_read_P6", revision);
    pin[P2] = Luos_CreateContainer(rx_digit_write_cb, STATE_MOD, "digit_write_P2", revision);
    pin[P3] = Luos_CreateContainer(rx_digit_write_cb, STATE_MOD, "digit_write_P3", revision);
    pin[P4] = Luos_CreateContainer(rx_digit_write_cb, STATE_MOD, "digit_write_P4", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void GpioDev_Loop(void)
{
}

static void rx_digit_read_cb(container_t *container, msg_t *msg)
{
    if (msg->header.cmd == ASK_PUB_CMD)
    {
        msg_t pub_msg;
        // fill the message infos
        pub_msg.header.cmd = IO_STATE;
        pub_msg.header.target_mode = ID;
        pub_msg.header.target = msg->header.source;
        pub_msg.header.size = sizeof(char);

        if (container == pin[P5])
        {
            pub_msg.data[0] = (char)(HAL_GPIO_ReadPin(P5_GPIO_Port, P5_Pin) > 0);
        }
        else if (container == pin[P6])
        {
            pub_msg.data[0] = (char)(HAL_GPIO_ReadPin(P6_GPIO_Port, P6_Pin) > 0);
        }
        else if (container == pin[P7])
		{
			pub_msg.data[0] = (char)(HAL_GPIO_ReadPin(P7_GPIO_Port, P7_Pin) > 0);
		}
        else if (container == pin[P8])
		{
			pub_msg.data[0] = (char)(HAL_GPIO_ReadPin(P8_GPIO_Port, P8_Pin) > 0);
		}
        else if (container == pin[P9])
		{
			pub_msg.data[0] = (char)(HAL_GPIO_ReadPin(P9_GPIO_Port, P9_Pin) > 0);
		}
        else if (container == pin[P1])
		{
			pub_msg.data[0] = (char)(HAL_GPIO_ReadPin(P1_GPIO_Port, P1_Pin) > 0);
		}
        else
        {
            return;
        }
        Luos_SendMsg(container, &pub_msg);
        return;
    }
}

static void rx_digit_write_cb(container_t *container, msg_t *msg)
{
    if (msg->header.cmd == IO_STATE)
    {
        if (container == pin[P2])
        {
            HAL_GPIO_WritePin(P2_GPIO_Port, P2_Pin, msg->data[0]);
        }
        if (container == pin[P3])
        {
            HAL_GPIO_WritePin(P3_GPIO_Port, P3_Pin, msg->data[0]);
        }
        if (container == pin[P4])
        {
            HAL_GPIO_WritePin(P4_GPIO_Port, P4_Pin, msg->data[0]);
        }
    }
}
