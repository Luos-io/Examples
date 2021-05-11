/******************************************************************************
 * @file gate
 * @brief Container gate
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/

#include "gate.h"
#include "json_mnger.h"
#include <stdio.h>
#include "json_pipe.h"
#include "json_alloc.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
container_t *gate;
/*******************************************************************************
 * Function
 ******************************************************************************/
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Gate_Init(void)
{
    revision_t revision = {.unmap = REV};
    json_pipe_init();
    gate = Luos_CreateContainer(0, GATE_MOD, "gate", revision);
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Gate_Loop(void)
{
    static short previous_id               = -1;
    static unsigned int keepAlive          = 0;
    static volatile uint8_t detection_done = 0;
    static char state                      = 0;
    char *tx_json                          = json_alloc_get_tx_buf();

    if (RoutingTB_IDFromContainer(gate) == 0)
    {
        // We don't have any ID, meaning no detection occure or detection is occuring.
        if (previous_id == -1)
        {
            // This is the start period, we have to make a detection.
            // Be sure the network is powered up 20 ms before starting a detection
#ifndef NODETECTION
            if (Luos_GetSystick() > 20)
            {
                // No detection occure, do it
                RoutingTB_DetectContainers(gate);
            }
#endif
        }
        else
        {
            // someone is making a detection, let it finish.
            // reset the previous_id state to be ready to setup container at the end of detection
            previous_id = 0;
        }
    }
    else
    {
        if (detection_ask)
        {
            detection_done = 0;
        }
        // Check if there is a dead container
        if (gate->ll_container->dead_container_spotted)
        {
            exclude_container_to_json(gate->ll_container->dead_container_spotted, tx_json);
            gate->ll_container->dead_container_spotted = 0;
        }
        if (detection_done)
        {
            state = !state;
            format_data(gate, tx_json);
            if (tx_json[0] != '\0')
            {
                keepAlive = 0;
            }
            else
            {
                if (keepAlive > 200)
                {
                    sprintf(tx_json, "{}\n");
                    tx_json = json_alloc_set_tx_task(strlen(tx_json));
                }
                else
                {
                    keepAlive++;
                }
            }
        }
        // check if serial input messages ready and convert it into a luos message
        send_cmds(gate);
        if (detection_done)
        {
            collect_data(gate);
        }
        if (detection_ask)
        {
            // reinit Json buffer.
            json_alloc_reinit();
            tx_json = json_alloc_get_tx_buf();
            // Run detection
            RoutingTB_DetectContainers(gate);
            // Create Json from container
            routing_table_to_json(tx_json);
            detection_done = 1;
            detection_ask  = 0;
        }
    }
}
