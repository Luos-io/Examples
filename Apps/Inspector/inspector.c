/******************************************************************************
 * @file gate
 * @brief Service gate
 * @author Luos
 ******************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include "inspector.h"
#include "pipe_link.h"
#include "data_manager.h"
#include "luos_list.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*******************************************************************************
 * Variables
 ******************************************************************************/
service_t *inspector;
uint8_t rtb_ask         = 1;
static uint16_t pipe_id = 0;
/*******************************************************************************
 * Function
 ******************************************************************************/
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Inspector_Init(void)
{
    revision_t revision = {.major = 1, .minor = 0, .build = 0};
    // inspector service creation
    inspector = Luos_CreateService(0, INSPECTOR_TYPE, "inspector", revision);
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Inspector_Loop(void)
{
    // check if the while network is detected
    if (Luos_IsNodeDetected())
    {
        // Network have been detected, We are good to go
        if (pipe_id == 0)
        {
            // We dont have spotted any pipe yet. Try to find one
            pipe_id = PipeLink_Find(inspector);
            // send to Robus a flag in order not to filter the messages
            Luos_SetFilterState(false);
        }
        // check if we have messages from pipe
        DataManager_GetPipeMsg(inspector);
    }
    else
    {
        // if the network is not yet detected reset the pipe id
        pipe_id = 0;
        // send to Robus a flag in order not to filter the messages
        Luos_SetFilterState(true);
        rtb_ask = 1;
    }
}
/******************************************************************************
 * @brief inspector msghandler
 * @param service pointer, msg received
 * @return None
 ******************************************************************************/
static void Inspector_MsgHandler(service_t *service, msg_t *msg)
{
    // if we receive a message send it directly to pipe
    if ((pipe_id > 0) && (RoutingTB_IDFromService(inspector) != msg->header.target) && (pipe_id != msg->header.target))
    {
        // the first time sent the routing table
        if (rtb_ask)
        {
            // store the address of the RoutingTB
            routing_table_t *routing_table = RoutingTB_Get();
            PipeLink_Send(inspector, routing_table, ((RoutingTB_GetLastEntry() + 1) * sizeof(routing_table_t)));
            rtb_ask = 0;
            return;
        }
        // send every message received
        PipeLink_Send(inspector, msg->stream, sizeof(uint8_t) * (msg->header.size + sizeof(header_t)));
        return;
    }
}
