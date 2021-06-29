/******************************************************************************
 * @file gate
 * @brief Service gate
 * @author Luos
 ******************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include "gate_config.h"
#include "gate.h"
#include "data_manager.h"
#include "convert.h"
#include "pipe_link.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
container_t *gate;
char detection_ask      = 0;
time_luos_t update_time = GATE_REFRESH_TIME_S;
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
    revision_t revision = {.Major = 1, .Minor = 0, .Build = 0};
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
            pipe_id = PipeLink_Find(gate);
        }
        if (gate_running && !detection_ask)
        {
            // Manage input and output data
            static uint32_t last_time = 0;
            if (Luos_GetSystick() - last_time >= TimeOD_TimeTo_ms(update_time))
            {
                last_time = Luos_GetSystick();
                DataManager_Run(gate);
            }
        }
        else
        {
            DataManager_RunPipeOnly(gate);
        }
        if (detection_ask)
        {
            // Run detection
            RoutingTB_DetectContainers(gate);
            pipe_id = PipeLink_Find(gate);
            // Create data from container
            Convert_RoutingTableData(gate);
#ifndef GATE_POLLING
            // Set update frequency
            DataManager_collect(gate);
#endif
            gate_running  = true;
            detection_ask = 0;
        }
    }
}
