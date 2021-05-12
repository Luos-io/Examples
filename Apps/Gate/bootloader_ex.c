/******************************************************************************
 * @file Bootloader extensions
 * @brief Bootloader functionnalities for luos framework
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "bootloader_ex.h"
#include "bootloader.h"

#include "gate.h"
#include "stm32f0xx_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void LuosBootloader_GateRcv(msg_t *msg)
{
    char boot_json[256] = "\0";
    uint8_t response_cmd = msg->data[0];

    switch (response_cmd)
    {
    case BOOTLOADER_READY_RESP:
        sprintf(boot_json, "{\"bootloader\":{\"response\":%d}}\n", BOOTLOADER_READY_RESP);
        break;

    case BOOTLOADER_BIN_HEADER_RESP:
        sprintf(boot_json, "{\"bootloader\":{\"response\":%d}}\n", BOOTLOADER_BIN_HEADER_RESP);
        break;

    default:
        break;
    }

    json_send(boot_json);
}