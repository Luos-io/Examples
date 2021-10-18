/******************************************************************************
 * @file data manager
 * @brief data manager for inspector
 * @author Luos
 ******************************************************************************/
#include "pipe_link.h"
#include "data_manager.h"
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
 * @brief function to poll the external messages from the pipe
 * @param service pointer
 * @return None
 ******************************************************************************/
void DataManager_GetPipeMsg(service_t *service)
{
    msg_t *data_msg;
    if (Luos_ReadFromService(service, PipeLink_GetId(), &data_msg) == SUCCEED)
    {
        // This message is a command from pipe
        if (data_msg->header.cmd == PARAMETERS)
        {
            // link the address of the streaming channel L2P
            int pointer;
            memcpy(&pointer, data_msg->data, sizeof(void *));
            PipeLink_SetStreamingChannel((void *)pointer);
        }
    }
}
