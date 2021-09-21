/******************************************************************************
 * @file pipe_link
 * @brief Manage the communication with a pipe.
 * @author Luos
 ******************************************************************************/
#include "pipe_link.h"
#include "streaming.h"

short pipe_id                             = 0;
streaming_channel_t *pipeStreamingChannel = 0;

void PipeLink_Send(service_t *service, void *data, uint32_t size)
{
    LUOS_ASSERT(pipe_id > 0);
    msg_t msg;
    msg.header.target      = pipe_id;
    msg.header.cmd         = SET_CMD;
    msg.header.target_mode = IDACK;
    if (pipeStreamingChannel == 0)
    {
        // We are not using localhost send the entire data trough the Luos network
        Luos_SendData(service, &msg, data, size);
    }
    else
    {
        // We have a localhost pipe
        // Copy the data directly into the local streaming channel without passing by Luos.
        Stream_PutSample(pipeStreamingChannel, data, size);
        // Send a void set_cmd to strat data transmission on pipe.
        msg.header.size = 0;
        Luos_SendMsg(service, &msg);
    }
}

short PipeLink_Find(service_t *service)
{
    pipe_id = RoutingTB_IDFromType(PIPE_TYPE);
    if (pipe_id > 0)
    {
        //We find one, ask it to auto-update at 1000Hz
        msg_t msg;
        msg.header.target      = pipe_id;
        msg.header.target_mode = IDACK;
        time_luos_t time       = TimeOD_TimeFrom_s(0.001f);
        TimeOD_TimeToMsg(&time, &msg);
        msg.header.cmd = UPDATE_PUB;
        while (Luos_SendMsg(service, &msg) != SUCCEED)
            ;

        // Check if pipe is localhost
        if (RoutingTB_NodeIDFromID(pipe_id) == RoutingTB_NodeIDFromID(service->ll_service->id))
        {
            // This is a localhost pipe
            // Ask for a Streaming channel
            msg_t msg;
            msg.header.target      = pipe_id;
            msg.header.target_mode = IDACK;
            msg.header.cmd         = PARAMETERS;
            msg.header.size        = 0;
            while (Luos_SendMsg(service, &msg) != SUCCEED)
                ;
        }
    }
    return pipe_id;
}

void PipeLink_Reset(service_t *service)
{
    LUOS_ASSERT(pipe_id > 0);
    msg_t msg;
    msg.header.target      = pipe_id;
    msg.header.cmd         = REINIT;
    msg.header.target_mode = IDACK;
    msg.header.size        = 0;
    Luos_SendMsg(service, &msg);
}

void PipeLink_SetStreamingChannel(void *streamingChannel)
{
    pipeStreamingChannel = streamingChannel;
}

short PipeLink_GetId(void)
{
    return pipe_id;
}
