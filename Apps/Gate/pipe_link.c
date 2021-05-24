#include "pipe_link.h"
#include "luos.h"

short pipe_id = 0;

void send_to_pipe(container_t *service, void *data, uint32_t size)
{
    LUOS_ASSERT(pipe_id > 0);
    msg_t msg;
    msg.header.target      = pipe_id;
    msg.header.cmd         = SET_CMD;
    msg.header.target_mode = IDACK;
    Luos_SendData(service, &msg, data, size);
}

short find_pipe(container_t *service)
{
    pipe_id = RoutingTB_IDFromType(PIPE_MOD);
    if (pipe_id > 0)
    {
        //We find one, ask it to auto-update at 1000Hz
        msg_t msg;
        msg.header.target      = pipe_id;
        msg.header.target_mode = IDACK;
        time_luos_t time       = TimeOD_TimeFrom_s(0.001f);
        TimeOD_TimeToMsg(&time, &msg);
        msg.header.cmd = UPDATE_PUB;
        Luos_SendMsg(service, &msg);
    }
    return pipe_id;
}

short get_pipe_id(void)
{
    return pipe_id;
}