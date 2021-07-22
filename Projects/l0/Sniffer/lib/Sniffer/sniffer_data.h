/******************************************************************************
 * @file sniffer_buffer
 * @brief sniffer data management
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include <stdio.h>
#include "sniffer_com.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define SNIFFER_BUFFER_SIZE 2 * MSG_BUFFER_SIZE
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
uint8_t *SnifferData_GetBuffer(void);
void SnifferData_PutMsg(uint64_t time, msg_t *msg);
streaming_channel_t *get_Sniffer_StreamChannel();
streaming_channel_t *create_Sniffer_StreamChannel();
void SnifferData_SendStat(void);
void SnifferData_SendInit(void);