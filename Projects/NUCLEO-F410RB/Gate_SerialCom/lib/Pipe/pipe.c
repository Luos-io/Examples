/******************************************************************************
 * @file pipe
 * @brief driver example a simple pipe
 * @author Luos
 * @version 0.1.0
 ******************************************************************************/
#include <stdbool.h>
#include "pipe.h"
#include "streaming.h"
#include "pipe_com.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
streaming_channel_t P2L_StreamChannel;
streaming_channel_t L2P_StreamChannel;
/*******************************************************************************
 * Function
 ******************************************************************************/
static void Pipe_MsgHandler(container_t *container, msg_t *msg);
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Pipe_Init(void)
{
    revision_t revision = {.unmap = REV};
    Luos_CreateContainer(Pipe_MsgHandler, PIPE_MOD, "Pipe_mod", revision);
    PipeCom_Init();
    P2L_StreamChannel = Stream_CreateStreamingChannel(PipeBuffer_GetP2LBuffer(), PIPE_TO_LUOS_BUFFER_SIZE, 1);
    L2P_StreamChannel = Stream_CreateStreamingChannel(PipeBuffer_GetL2PBuffer(), LUOS_TO_PIPE_BUFFER_SIZE, 1);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Pipe_Loop(void)
{
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this container
 * @param Container destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Pipe_MsgHandler(container_t *container, msg_t *msg)
{
    uint8_t *data = 0;
    uint16_t size = 0;
    if (msg->header.cmd == GET_CMD)
    {
        if (true == PipeBuffer_GetP2LTask(&data, &size))
        {
            // fill the message infos
            msg_t pub_msg;
            pub_msg.header.cmd         = SET_CMD;
            pub_msg.header.target_mode = ID;
            pub_msg.header.target      = msg->header.source;
            pub_msg.header.size        = size;
            Stream_AddAvailableSampleNB(&P2L_StreamChannel, size);
            Luos_SendStreaming(container, &pub_msg, &P2L_StreamChannel);
        }
    }
    else if (msg->header.cmd == SET_CMD)
    {
        uint16_t size = 0;
        if (msg->header.size > 0)
        {
            Luos_ReceiveStreaming(container, msg, &L2P_StreamChannel);
        }
        if (PipeCom_SendL2PPending() == false)
        {
            size = Stream_GetAvailableSampleNBUntilEndBuffer(&L2P_StreamChannel);
            if (size > 0)
            {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
                PipeCom_SendL2P(L2P_StreamChannel.sample_ptr, size);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
            }
        }
    }
    else if (msg->header.cmd == PARAMETERS)
    {
        // return output streaming channel adress
        msg_t pub_msg;
        pub_msg.header.cmd         = PARAMETERS;
        pub_msg.header.target_mode = IDACK;
        pub_msg.header.target      = msg->header.source;
        pub_msg.header.size        = sizeof(void *);
        int value                  = (int)&L2P_StreamChannel;
        memcpy(pub_msg.data, &value, sizeof(void *));
        Luos_SendMsg(container, &pub_msg);
    }
    else if (msg->header.cmd == REINIT)
    {
         L2P_StreamChannel.sample_ptr = L2P_StreamChannel.data_ptr;
    }
}

streaming_channel_t *get_L2P_StreamChannel()
{
    return &L2P_StreamChannel;
}