/******************************************************************************
 * @file data_manager
 * @brief Manage data conversion strategy.
 * @author Luos
 ******************************************************************************/
#include <stdbool.h>
#include "convert.h"
#include "gate_config.h"
#include "data_manager.h"
#include "pipe_link.h"
#include "bootloader_ex.h"

static void DataManager_Format(service_t *service);

// This function will manage msg collection from sensors
void DataManager_collect(service_t *service)
{
    msg_t update_msg;
#ifdef GATE_POLLING
    update_msg.header.cmd         = GET_CMD;
    update_msg.header.target_mode = ID;
    update_msg.header.size        = 0;
#else
    update_msg.header.target_mode = IDACK;
#endif
    // ask services to publish datas
    for (uint8_t i = 1; i <= RoutingTB_GetLastService(); i++)
    {
        // Check if this service is a sensor
        if ((RoutingTB_ServiceIsSensor(RoutingTB_TypeFromID(i))) || (RoutingTB_TypeFromID(i) >= LUOS_LAST_TYPE))
        {
#ifdef GATE_POLLING
            // This service is a sensor so create a msg and send it
            update_msg.header.target = i;
            Luos_SendMsg(service, &update_msg);
#ifdef GATE_TIMEOUT
            // Get the current number of message available
            int back_nbr_msg = Luos_NbrAvailableMsg();
            // Get the current time
            uint32_t send_time = Luos_GetSystick();
            // Wait for a reply before continuing
            while ((back_nbr_msg == Luos_NbrAvailableMsg()) & (send_time == Luos_GetSystick()))
            {
                Luos_Loop();
            }
#endif
#else
            // This container is a sensor so create a msg to enable auto update
            update_msg.header.target = i;
            TimeOD_TimeToMsg(&update_time, &update_msg);
            update_msg.header.cmd = UPDATE_PUB;
            Luos_SendMsg(service, &update_msg);
#endif
        }
    }
    // wait a little bit for the first reply
    uint32_t start_time = Luos_GetSystick();
    while ((start_time == Luos_GetSystick()) && (Luos_NbrAvailableMsg() == 0))
        ;
}

// This function manage entirely data conversion
void DataManager_Run(service_t *service)
{
    // Check if there is a dead service.
    if (service->ll_service->dead_service_spotted)
    {
        Convert_ExcludedServiceData(service);
        RoutingTB_RemoveOnRoutingTable(service->ll_service->dead_service_spotted);
        // Reset spotted dead service
        service->ll_service->dead_service_spotted = 0;
    }
#ifdef GATE_POLLING
    DataManager_collect(service);
#endif
    DataManager_Format(service);
}
// This function manage only commande incoming from pipe
void DataManager_RunPipeOnly(service_t *service)
{
    msg_t *data_msg;
    while (Luos_ReadFromService(service, PipeLink_GetId(), &data_msg) == SUCCEED)
    {
        // This message is a command from pipe
        // Convert the received data into Luos commands
        static char data_cmd[GATE_BUFF_SIZE];
        if (data_msg->header.cmd == PARAMETERS)
        {
            int pointer;
            memcpy(&pointer, data_msg->data, sizeof(void *));
            PipeLink_SetStreamingChannel((void *)pointer);
        }
        else
        {
            static uint32_t data_size = 0;
            data_size += Luos_ReceiveData(service, data_msg, &data_cmd[data_size]);
            if (data_size > 2)
            {

                int missing_data = Convert_CheckDataIntegrity(data_cmd, data_size);
                if (missing_data == 0)
                {
                    // We finish to receive this data, execute the received command

                    Convert_DataToLuos(service, data_cmd);
                    data_size = 0;
                }
            }
            else
            {
                data_size = 0;
            }
        }
    }
    if (Luos_ReadMsg(service, &data_msg) == SUCCEED)
    {
        // Check if a node send a end detection
        if (data_msg->header.cmd == END_DETECTION)
        {
            // Find a pipe
            PipeLink_Find(service);
            if (gate_running == PREPARING)
            {
                // Generate routing table datas
                Convert_RoutingTableData(service);
                // Run the gate
                gate_running = RUNNING;
#ifndef GATE_POLLING
                first_conversion = true;
#endif
            }
        }
    }
}

// This function will create a data string for services datas
void DataManager_Format(service_t *service)
{
    static uint32_t FirstNoReceptionDate = 0;
    char data[GATE_BUFF_SIZE];
    char *data_ptr  = data;
    msg_t *data_msg = 0;
    uint8_t data_ok = false;
    if ((Luos_NbrAvailableMsg() > 0))
    {
        // Init the data string
        data_ptr += Convert_StartData(data_ptr);
        // loop into services.
        int i = 0;
        while (RoutingTB_GetMode(i) != CLEAR)
        {
            if (RoutingTB_GetMode(i) == SERVICE)
            {
                if (Luos_ReadFromService(service, RoutingTB_GetServiceID(i), &data_msg) == SUCCEED)
                {
                    // check if this is an assert
                    if (data_msg->header.cmd == ASSERT)
                    {
                        luos_assert_t assertion;
                        memcpy(assertion.unmap, data_msg->data, data_msg->header.size);
                        assertion.unmap[data_msg->header.size] = '\0';
                        Convert_AssertToData(service, data_msg->header.source, assertion);
                        i++;
                        continue;
                    }
                    // check if a node send a bootloader message
                    if (data_msg->header.cmd == BOOTLOADER_RESP)
                    {
                        Bootloader_LuosToJson(service, data_msg);
                        continue;
                    }
                    // check if a node send a end detection
                    if (data_msg->header.cmd == END_DETECTION)
                    {
                        // find a pipe
                        PipeLink_Find(service);
                        i++;
                        continue;
                    }
                    // Check if this is a message from pipe
                    if (data_msg->header.source == PipeLink_GetId())
                    {

                        if (data_msg->header.cmd == PARAMETERS)
                        {
                            int pointer;
                            memcpy(&pointer, data_msg->data, sizeof(void *));
                            PipeLink_SetStreamingChannel((void *)pointer);
                        }
                        else
                        {
                            do
                            {
                                // This message is a command from pipe
                                static char data_cmd[GATE_BUFF_SIZE];
                                static char *pipe_data_ptr = data_cmd;
                                // Convert the received data into Luos commands
                                static uint32_t data_size = 0;

                                data_size += Luos_ReceiveData(service, data_msg, pipe_data_ptr);
                                if (data_size > 0)
                                {
                                    if (data_size < 2)
                                    {
                                        // less than 8 bytes are probably glitchs, reset
                                        pipe_data_ptr = data_cmd;
                                        data_size     = 0;
                                    }
                                    else
                                    {
                                        // We finish to receive this data,
                                        // This could be only a piece of the actual command. We have to validate the complitude of the data
                                        int missing_data = Convert_CheckDataIntegrity(data_cmd, data_size);
                                        if (missing_data == 0)
                                        {

                                            // Execute the received command
                                            if (data_msg->header.cmd == SET_CMD)
                                            {
                                                Convert_DataToLuos(service, data_cmd);
                                            }
                                            // reinit the data pointer to the begining of the buffer

                                            pipe_data_ptr = data_cmd;
                                            data_size     = 0;
                                        }
                                        else if (missing_data > 0)
                                        {

                                            // This data is incomplete, move the data pointer into the buffer to add the next reception after
                                            pipe_data_ptr = &data_cmd[data_size];
                                        }
                                        else if (missing_data < 0)
                                        {
                                            // This is an error drop the message
                                            pipe_data_ptr = data_cmd;
                                            data_size     = 0;
                                        }
                                    }
                                }
                            } while (Luos_ReadFromService(service, PipeLink_GetId(), &data_msg) == SUCCEED);
                            i++;
                            continue;
                        }
                    }
                    // get the source of this message
                    // Create service description
                    char *alias;
                    alias = RoutingTB_AliasFromId(data_msg->header.source);
                    LUOS_ASSERT(alias != 0);
                    data_ok = true;
                    data_ptr += Convert_StartServiceData(data_ptr, alias);
                    // Convert all msgs from this service into data
                    do
                    {
                        data_ptr += Convert_MsgToData(data_msg, data_ptr);
                    } while (Luos_ReadFromService(service, data_msg->header.source, &data_msg) == SUCCEED);

                    data_ptr += Convert_EndServiceData(data_ptr);
                    LUOS_ASSERT((data_ptr - data) < GATE_BUFF_SIZE);
                }
            }
            i++;
        }
        if (data_ok)
        {
            Convert_EndData(service, data, data_ptr);
            FirstNoReceptionDate = 0;
        }
        else
        {
            // We don't receive anything.
            // After 1s void reception send void data allowing client to send commands (because client could be synchronized to reception).
            if (FirstNoReceptionDate == 0)
            {
                FirstNoReceptionDate = Luos_GetSystick();
            }
            else if (Luos_GetSystick() - FirstNoReceptionDate > 1000)
            {
                Convert_VoidData(service);
            }
        }
    }
    else
    {
        // We don't receive anything.
        // After 1s void reception send void data allowing client to send commands (because client could be synchronized to reception).
        if (FirstNoReceptionDate == 0)
        {
            FirstNoReceptionDate = Luos_GetSystick();
        }
        else if (Luos_GetSystick() - FirstNoReceptionDate > 1000)
        {
            Convert_VoidData(service);
        }
    }
}
