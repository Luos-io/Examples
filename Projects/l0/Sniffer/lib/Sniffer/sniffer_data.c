/******************************************************************************
 * @file sniffer_data
 * @brief sniffer data management
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "sniffer_data.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
uint8_t Sniffer_Buffer[SNIFFER_BUFFER_SIZE];
streaming_channel_t Sniffer_StreamChannel;
/*******************************************************************************
 * Function
 ******************************************************************************/
void to_bytes(uint64_t num, uint8_t *res);
/******************************************************************************
 * @brief give the address of the streaming buffer
 * @param None
 * @return sniffer buffer 
 ******************************************************************************/
uint8_t *SnifferData_GetBuffer(void)
{
    return &Sniffer_Buffer[0];
}
/******************************************************************************
 * @brief Put a full formatized message in the streaming channel
 * @param timestamp, message pointer
 * @return None
 ******************************************************************************/
void SnifferData_PutMsg(uint64_t time, msg_t *msg)
{
    uint16_t size;
    uint8_t res[8] = {0};

    Sniffer_StreamChannel = *get_Sniffer_StreamChannel();
    //convert timestamp into an array of bytes
    to_bytes(time, &res[0]);
    //send timestamp
    Stream_PutSample(&Sniffer_StreamChannel, res, 8);
    //calculate the data size - check if it is bigger than 128 bytes
    size = (msg->header.size < MAX_DATA_MSG_SIZE) ? msg->header.size + sizeof(header_t) : MAX_DATA_MSG_SIZE + sizeof(header_t);
    //store message in streaming channel
    Stream_PutSample(&Sniffer_StreamChannel, msg->stream, size);
    //end of message

    sprintf((char *)res, "~~~");
    Stream_PutSample(&Sniffer_StreamChannel, res, 3);
}
/******************************************************************************
 * @brief Put the statistics in streaming channel
 * @param None
 * @return None
 ******************************************************************************/
void SnifferData_SendStat(void)
{
    uint8_t res[8] = {0};

    Sniffer_StreamChannel = *get_Sniffer_StreamChannel();
    //identification msg
    sprintf((char *)res, "stat");
    Stream_PutSample(&Sniffer_StreamChannel, res, 4);
    //Crc error num
    //transform error num to an array of bytes
    to_bytes(ctx.sniffer.crc_error_count, &res[0]);
    Stream_PutSample(&Sniffer_StreamChannel, res, 8);
    //Corrupted msgs
    to_bytes(ctx.sniffer.corruption_count, &res[0]);
    Stream_PutSample(&Sniffer_StreamChannel, res, 8);
    SnifferCom_Send(Sniffer_StreamChannel.sample_ptr, 20);
}
/******************************************************************************
 * @brief Send the sniffer initialization response
 * @param None
 * @return None
 ******************************************************************************/
void SnifferData_SendInit(void)
{
    uint8_t str[4] = {0};

    Sniffer_StreamChannel = *get_Sniffer_StreamChannel();
    sprintf((char *)str, "yes");
    //copy string in streaming channel
    Stream_PutSample(&Sniffer_StreamChannel, str, 3);
    //send response
    SnifferCom_Send(Sniffer_StreamChannel.sample_ptr, 3);
}

/******************************************************************************
 * @brief create the sniffer streaming channel
 * @param 
 * @return Streaming channel address
 ******************************************************************************/
streaming_channel_t *create_Sniffer_StreamChannel()
{
    //create streaming channel
    Sniffer_StreamChannel = Stream_CreateStreamingChannel(SnifferData_GetBuffer(), SNIFFER_BUFFER_SIZE, 1);
    return &Sniffer_StreamChannel;
}

/******************************************************************************
 * @brief give the sniffer streaming channel
 * @param 
 * @return Streaming channel address
 ******************************************************************************/
streaming_channel_t *get_Sniffer_StreamChannel()
{
    return &Sniffer_StreamChannel;
}
/******************************************************************************
 * @brief converts a uint64_t to an array of bytes
 * @param uint64_t to be converted, pointer to the array of bytes
 * @return None
 ******************************************************************************/
void to_bytes(uint64_t num, uint8_t *res)
{
    uint8_t *p = (uint8_t *)&num;

    for (uint8_t i = 0; i < 8; i++)
    {
        res[i] = *(p + i);
    }
}
