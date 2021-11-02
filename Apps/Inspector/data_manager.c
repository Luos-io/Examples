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
uint8_t inspector_state = STARTED;
msg_t assert_msg[5];

/*******************************************************************************
 * Function
 ******************************************************************************/
/******************************************************************************
 * @brief function to send the full routing table
 * @param service pointer
 * @return None
 ******************************************************************************/
void DataManager_SendRoutingTB(service_t *service)
{
    uint8_t *data;
    // store the address of the RoutingTB
    routing_table_t *routing_table = RoutingTB_Get();
    msg_t msg;
    msg.header.protocol    = 0;
    msg.header.target      = DEFAULTID;
    msg.header.target_mode = ID;
    msg.header.source      = RoutingTB_IDFromService(service);
    msg.header.cmd         = RTB_CMD;
    msg.header.size        = RoutingTB_GetLastEntry() * sizeof(routing_table_t);

    memcpy(data, msg.stream, sizeof(header_t));
    memcpy(&data[sizeof(header_t)], routing_table, msg.header.size);

    PipeLink_Send(service, data, msg.header.size + sizeof(header_t));
}
/******************************************************************************
 * @brief extract the command value for the pipe messages
 * @param service pointer
 * @return None
 ******************************************************************************/
uint8_t DataManager_ExtractCommand(msg_t *msg)
{
    return msg->data[4];
}
/******************************************************************************
 * @brief function to pull the external messages from the pipe
 * @param service pointer
 * @return None
 ******************************************************************************/
void DataManager_GetPipeMsg(service_t *service, msg_t *data_msg)
{
    int *pointer;
    msg_t msg;
    uint8_t cmd;

    if (data_msg->header.cmd == PARAMETERS)
    {
        // link the address of the streaming channel L2P
        memcpy(&pointer, data_msg->data, sizeof(void *));
        PipeLink_SetStreamingChannel((void *)pointer);
        return;
    }

    cmd = DataManager_ExtractCommand(data_msg);
    //  This message is a command from pipe
    switch (cmd)
    {
        case GET_RTB:
            // first message for the inspector
            // send the routing table using pipe
            DataManager_SendRoutingTB(service);
            break;
        case SERVICE_START:
            // if we receive a start we should desactivate the filtering
            Luos_SetFilterState(false, RoutingTB_IDFromService(service));
            inspector_state = STARTED;
            break;
        case SERVICE_STOP:
            // if we receive a stop we should reactivate the filtering
            Luos_SetFilterState(true, RoutingTB_IDFromService(service));
            inspector_state = STOPPED;
            break;
        case LUOS_STATISTICS:
            // extract service that we want the stats
            msg.header.target      = (data_msg->data[8] << 8) + data_msg->data[7];
            msg.header.target_mode = ID;
            msg.header.cmd         = LUOS_STATISTICS;
            msg.header.size        = 0;
            Luos_SendMsg(service, &msg);
            break;
        case LUOS_REVISION:
            // extract service that we want the luos revision
            msg.header.target      = (data_msg->data[8] << 8) + data_msg->data[7];
            msg.header.target_mode = ID;
            msg.header.cmd         = LUOS_REVISION;
            msg.header.size        = 0;
            Luos_SendMsg(service, &msg);
            break;
        case REVISION:
            // extract service that we want the firmware revision
            msg.header.target      = (data_msg->data[8] << 8) + data_msg->data[7];
            msg.header.target_mode = ID;
            msg.header.cmd         = REVISION;
            msg.header.size        = 0;
            Luos_SendMsg(service, &msg);
            break;
        case VERBOSE:
            break;
        case GET_ASSERT:
        // not yet implemented
        default:
            break;
    }
}
/******************************************************************************
 * @brief add a new assert message to the assert buffer
 * @param service pointer
 * @return None
 ******************************************************************************/
void DataManager_AddAssertMsg(msg_t *msg)
{
}
/******************************************************************************
 * @brief get if the inspector is started or stopped
 * @param service pointer
 * @return None
 ******************************************************************************/
uint8_t DataManager_GetInspectorState(void)
{
    return inspector_state;
}
