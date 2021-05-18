/******************************************************************************
 * @file gate
 * @brief Container gate
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/

#include "gate.h"
#include "luos_to_json.h"
#include "json_pipe.h"
#include "json_alloc.h"
#include "json_to_luos.h"
#include "convert.h"
#include <stdio.h>
#include <stdbool.h>

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
    static short previous_id          = -1;
    static unsigned int keepAlive     = 0;
    static volatile bool gate_running = false;
    char *tx_json                     = json_alloc_get_tx_buf();

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
    }
    else
    {
        // Network have been detected, We are good to go
        // Check if there is a dead container
        if (gate->ll_container->dead_container_spotted)
        {
            // There is a dead container spotted by gate, manage it.
            exclude_container_to_json(gate->ll_container->dead_container_spotted, tx_json);
            gate->ll_container->dead_container_spotted = 0;
        }
        if (gate_running && !detection_ask)
        {
            // retrive and convert received data into Json
            static uint32_t last_time = 0;
            if (Luos_GetSystick() - last_time >= TimeOD_TimeTo_ms(get_update_time()))
            {
                last_time = Luos_GetSystick();
                luos_to_json(gate, tx_json);
            }
            // Check if we don't convert anything.
            if (tx_json[0] != '\0')
            {
                keepAlive = 0;
            }
            else
            {
                // We don't receive anything.
                // After 200 void reception send void Json allowing client to send commands.
                if (keepAlive > 200)
                {
                    //sprintf(tx_json, "{}\n");
                    //tx_json = json_alloc_set_tx_task(strlen(tx_json));
                }
                else
                {
                    keepAlive++;
                }
            }
        }
        // check if serial input messages ready and convert it into luos messages
        json_to_luos(gate);
        if (detection_ask)
        {
            // reinit Json buffer.
            json_alloc_reinit();
            tx_json = json_alloc_get_tx_buf();
            // Run detection
            RoutingTB_DetectContainers(gate);
            // Create Json from container
            routing_table_to_json(tx_json);
#ifndef GATE_POLLING
            // Set update frequecy
            collect_data(gate);
#endif
            gate_running  = true;
            detection_ask = 0;
        }
    }
}
