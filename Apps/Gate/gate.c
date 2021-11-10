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
service_t *gate;
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
    revision_t revision = {.major = 1, .minor = 0, .build = 0};
    gate                = Luos_CreateService(0, GATE_TYPE, "gate", revision);
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Gate_Loop(void)
{
#ifndef GATE_POLLING
    static uint8_t first_conversion = 1;
#endif
    static short pipe_id = 0;
#ifndef NODETECTION
    static short previous_id = -1;
#endif
    static volatile bool gate_running = false;
    static uint32_t last_time         = 0;

    // Check the detection status.
    if (RoutingTB_IDFromService(gate) == 0)
    {
#ifndef NODETECTION
        // We don't have any ID, meaning no detection occure or detection is occuring.
        if (previous_id == -1)
        {
            // This is the start period, we have to make a detection.
            // Be sure the network is powered up 20 ms before starting a detection
            if (Luos_GetSystick() > 20)
            {
                // No detection occure, do it
                RoutingTB_DetectServices(gate);
                previous_id = 0;
#ifndef GATE_POLLING
                first_conversion = 1;
                update_time      = TimeOD_TimeFrom_s(GATE_REFRESH_TIME_S);
#endif
            }
        }
#endif
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
            if ((Luos_GetSystick() - last_time >= TimeOD_TimeTo_ms(update_time)) && (Luos_GetSystick() > last_time))
            {
                last_time = Luos_GetSystick();
                DataManager_Run(gate);
#ifndef GATE_POLLING
                if (first_conversion == 1)
                {
                    // This is the first time we perform a convertion
                    // Evaluate the time needed to convert all the data of this configuration and update refresh rate
                    uint32_t execution_time = ((Luos_GetSystick() - last_time) * 2) + 2;
                    update_time             = TimeOD_TimeFrom_ms(execution_time);
                    // Update refresh rate for all services of the network
                    DataManager_collect(gate);
                    first_conversion = 0;
                }
#else
                update_time = 0.005;
#endif
            }
        }
        else
        {
            DataManager_RunPipeOnly(gate);
        }
        if (detection_ask)
        {
            // Run detection
            RoutingTB_DetectServices(gate);
            pipe_id = PipeLink_Find(gate);
            // Create data from service
            Convert_RoutingTableData(gate);
#ifndef GATE_POLLING
            // Set update frequency
            update_time = TimeOD_TimeFrom_s(GATE_REFRESH_TIME_S);
            DataManager_collect(gate);
            last_time        = Luos_GetSystick() + (uint32_t)(TimeOD_TimeTo_ms(GATE_REFRESH_TIME_S) / 2);
            first_conversion = 1;
#endif
            gate_running  = true;
            detection_ask = 0;
        }
    }
}
