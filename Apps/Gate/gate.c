/******************************************************************************
 * @file gate
 * @brief Container gate
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/

#include "gate.h"
#include "luos_to_json.h"
#include "json_to_luos.h"
#include "convert.h"
#include "gate_config.h"
#include <stdio.h>
#include <stdbool.h>
#include "pipe_link.h"

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
    gate                = Luos_CreateContainer(0, GATE_MOD, "gate", revision);
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Gate_Loop(void)
{
    static short pipe_id              = 0;
    static short previous_id          = -1;
    static volatile bool gate_running = false;

    // Check the detection status.
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
        pipe_id = 0;
    }
    else
    {
        // Network have been detected, We are good to go
        if (pipe_id == 0)
        {
            // We dont have spotted any pipe yet. Try to find one
            pipe_id = find_pipe(gate);
        }
        if (gate_running && !detection_ask)
        {
            // retrive and convert received data into Json
            static uint32_t last_time = 0;
            if (Luos_GetSystick() - last_time >= TimeOD_TimeTo_ms(get_update_time()))
            {
                last_time = Luos_GetSystick();
                luos_to_json(gate);
            }
        }
        if (detection_ask)
        {
            // Run detection
            RoutingTB_DetectContainers(gate);
            // Create Json from container
            routing_table_to_json(gate);
#ifndef GATE_POLLING
            // Set update frequecy
            collect_data(gate);
#endif
            gate_running  = true;
            detection_ask = 0;
        }
    }
}
