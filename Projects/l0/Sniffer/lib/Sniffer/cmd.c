#include "cmd.h"
#include <stdio.h>
#include "sniffer.h"
#include <string.h>

// There is no stack here we use the latest command
volatile uint8_t buf[CMD_BUFF_SIZE] = {0};
uint8_t state                       = 0;

char *get_cmd_buf(void)
{
    return (char *)buf;
}
/******************************************************************************
 * @brief func that treats the received cmd msgs, and specifies the sniffer state
 * @param None
 * @return None
 ******************************************************************************/

uint8_t Get_Sniffer_State(void)
{
    // check if we have a complete received command
    //check if this is a start sniffer cmd
    if (strstr((const char *)buf, "start") != NULL)
    {
        state = STARTED;
        for (uint8_t i = 0; i < CMD_BUFF_SIZE; i++)
        {
            buf[i] = 0;
        }
    }
    //check if this is a pause sniffer cmd
    else if (strstr((const char *)buf, "pause") != NULL)
    {
        for (uint8_t i = 0; i < CMD_BUFF_SIZE; i++)
        {
            buf[i] = 0;
        }
        state = PAUSED;
    }
    //check if this is a stop sniffer cmd
    else if (strstr((const char *)buf, "stop") != NULL)
    {
        state = STOPPED;
        for (uint8_t i = 0; i < CMD_BUFF_SIZE; i++)
        {
            buf[i] = 0;
        }
    }
    //check if this is a "is there a sniffer?" cmd
    else if (strstr((const char *)buf, "init") != NULL)
    {
        state = INIT;
        for (uint8_t i = 0; i < CMD_BUFF_SIZE; i++)
        {
            buf[i] = 0;
        }
    }
    return state;
}

void Set_Default_Sniffer_State(void)
{
    state = STOPPED;
}
