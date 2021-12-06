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
#include "routing_table.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
service_t *gate;
char detection_ask      = 0;
time_luos_t update_time = GATE_REFRESH_TIME_S;
bool gate_running       = false;
char end_detection      = 0;
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
    revision_t revision = {.major = 1, .minor = 0, .build = 1};
    gate                = Luos_CreateService(0, GATE_TYPE, "gate", revision);
#ifndef NODETECTION
    Luos_Detect(gate);
#endif
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Gate_Loop(void)
{
    static short pipe_id      = 0;
    static uint32_t last_time = 0;

    // Check the detection status.
    if (Luos_IsNodeDetected())
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
                if (end_detection)
                {
#ifndef GATE_POLLING
                    // This is the first time we perform a convertion
                    // Evaluate the time needed to convert all the data of this configuration and update refresh rate
                    uint16_t bigest_id = RoutingTB_BigestID();
                    if (bigest_id)
                    {
                        update_time = (float)RoutingTB_BigestID() * 0.001;
                    }
                    else
                    {
                        update_time = GATE_REFRESH_TIME_S;
                    }

                    // Update refresh rate for all services of the network
                    DataManager_collect(gate);
#else
                    update_time = 0.005;
#endif
                    end_detection = 0;
                }
                DataManager_Run(gate);
            }
        }
        else
        {
            // find messages from pipe
            DataManager_RunPipeOnly(gate);
            // Create data from service and send routing table
            Convert_RoutingTableData(gate);
            // run gate
            gate_running = true;
        }
        if (detection_ask)
        {
            // Run detection
            Luos_Detect(gate);
            pipe_id = 0;
#ifndef GATE_POLLING
            // Set update frequency
            update_time = TimeOD_TimeFrom_s(GATE_REFRESH_TIME_S);
            DataManager_collect(gate);
            last_time = Luos_GetSystick() + (uint32_t)(TimeOD_TimeTo_ms(GATE_REFRESH_TIME_S) / 2);
#endif
            gate_running  = false;
            detection_ask = 0;
        }
    }
    else
    {
        gate_running = false;
        pipe_id      = 0;
    }
}
