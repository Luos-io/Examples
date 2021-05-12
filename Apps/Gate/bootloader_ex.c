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
#include "json_mnger.h"

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
/******************************************************************************
 * @brief Process node responses and send them to the Host
 * @param luos message
 * @return None
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

    case BOOTLOADER_BIN_CHUNK_RESP:
        sprintf(boot_json, "{\"bootloader\":{\"response\":%d}}\n", BOOTLOADER_BIN_CHUNK_RESP);
        break;

    default:
        break;
    }

    json_send(boot_json);
}

/******************************************************************************
 * @brief Process Host commands and send them to the node
 * @param luos message
 * @return None
 ******************************************************************************/
void LuosBootloader_GateCmd(container_t *container, char *bin_data, cJSON *bootloader_json)
{
    if (cJSON_IsObject(cJSON_GetObjectItem(bootloader_json, "command")))
    {
        // command type
        char *cmd[8] = {
            "dummy",
            "start",
            "stop",
            "ready",
            "bin_header",
            "bin_chunk",
            "bin_end",
            "crc_test"};

        // get "command" json object
        cJSON *command_item = cJSON_GetObjectItem(bootloader_json, "command");
        // parse all relevant values in json object
        char *type = cJSON_GetStringValue(cJSON_GetObjectItem(command_item, "type"));
        uint8_t node_target = cJSON_GetObjectItem(command_item, "node")->valueint;

        // create a message to send to nodes
        msg_t boot_msg;
        boot_msg.header.target = node_target;    // first node of the network
        boot_msg.header.cmd = BOOTLOADER_CMD;    // bootloader cmd
        boot_msg.header.target_mode = NODEIDACK; // msg send to the node

        if (strcmp(type, cmd[BOOTLOADER_START]) == 0)
        {
            // send start command to bootloader app
            boot_msg.header.size = sizeof(char); //Our message only contains one character
            boot_msg.data[0] = BOOTLOADER_START;
            Luos_SendMsg(container, &boot_msg); //Now that we have the elements, send the message
        }

        if (strcmp(type, cmd[BOOTLOADER_STOP]) == 0)
        {
            // send stop command to bootloader app
            boot_msg.header.size = sizeof(char); //Our message only contains one character
            boot_msg.data[0] = BOOTLOADER_STOP;
            Luos_SendMsg(container, &boot_msg); //Now that we have the elements, send the message
        }

        if (strcmp(type, cmd[BOOTLOADER_READY]) == 0)
        {
            // send ready command to bootloader app
            boot_msg.header.size = sizeof(char); //Our message only contains one character
            boot_msg.data[0] = BOOTLOADER_READY;
            Luos_SendMsg(container, &boot_msg); //Now that we have the elements, send the message
        }

        if (strcmp(type, cmd[BOOTLOADER_BIN_HEADER]) == 0)
        {
            // send bin header command to bootloader app
            boot_msg.header.size = sizeof(char); //Our message only contains one character
            boot_msg.data[0] = BOOTLOADER_BIN_HEADER;
            Luos_SendMsg(container, &boot_msg); //Now that we have the elements, send the message
        }

        if (strcmp(type, cmd[BOOTLOADER_BIN_CHUNK]) == 0)
        {
            // find binary size in json header
            uint8_t binary_size = cJSON_GetObjectItem(command_item, "size")->valueint;

            // send bin chunk command to bootloader app
            boot_msg.data[0] = BOOTLOADER_BIN_CHUNK;
            int i = 0;
            // find the first \r of the current buf
            for (i = 0; i < JSON_BUFF_SIZE; i++)
            {
                if (bin_data[i] == '\r')
                {
                    i++;
                    break;
                }
            }
            if (i < JSON_BUFF_SIZE - 1)
            {
                boot_msg.header.size = binary_size + sizeof(char);
                memcpy(&(boot_msg.data[1]), &bin_data[i], binary_size);
                Luos_SendMsg(container, &boot_msg); //Now that we have the elements, send the message
            }
        }

        if (strcmp(type, cmd[BOOTLOADER_BIN_END]) == 0)
        {
            // send bin header command to bootloader app
            boot_msg.header.size = sizeof(char); //Our message only contains one character
            boot_msg.data[0] = BOOTLOADER_BIN_END;
            Luos_SendMsg(container, &boot_msg); //Now that we have the elements, send the message
        }
    }
}