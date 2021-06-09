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

static void DataManager_Format(container_t *service);

// This function will manage msg collection from sensors
void DataManager_collect(container_t *service)
{
    msg_t update_msg;
#ifdef GATE_POLLING
    update_msg.header.cmd         = ASK_PUB_CMD;
    update_msg.header.target_mode = ID;
    update_msg.header.size        = 0;
#else
    update_msg.header.target_mode = IDACK;
#endif
    // ask containers to publish datas
    for (uint8_t i = 1; i <= RoutingTB_GetLastContainer(); i++)
    {
        // Check if this container is a sensor
        if ((RoutingTB_ContainerIsSensor(RoutingTB_TypeFromID(i))) || (RoutingTB_TypeFromID(i) >= LUOS_LAST_TYPE))
        {
#ifdef GATE_POLLING
            // This container is a sensor so create a msg and send it
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
            // This contaiiner is a sensor so create a msg to enable auto update
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
void DataManager_Run(container_t *service)
{
    // Check if there is a dead container.
    if (service->ll_container->dead_container_spotted)
    {
        Convert_ExcludedContainerData(service);
        RoutingTB_RemoveOnRoutingTable(service->ll_container->dead_container_spotted);
        // Reset spotted dead container
        service->ll_container->dead_container_spotted = 0;
    }
#ifdef GATE_POLLING
    DataManager_collect(service);
#endif
    DataManager_Format(service);
}
// This function manage only commande incoming from pipe
void DataManager_RunPipeOnly(container_t *service)
{
    msg_t *data_msg;
    while (Luos_ReadFromContainer(service, PipeLink_GetId(), &data_msg) == SUCCEED)
    {
        // This message is a command from pipe
        // Convert the received data into Luos commands
        static char data_cmd[GATE_BUFF_SIZE];
        if (data_msg->header.cmd == PARAMETERS)
        {
            int pointer;
            memcpy(&pointer, data_msg->data, sizeof(void *));
            PipeLink_SetStreamingChannel((void *)pointer);
            continue;
        }
        if (Luos_ReceiveData(service, data_msg, data_cmd) == SUCCEED)
        {
            // We finish to receive this data, execute the received command
            Convert_DataToLuos(service, data_cmd);
        }
    }
}

// This function will create a data string for containers datas
void DataManager_Format(container_t *service)
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
        // loop into containers.
        int i = 0;
        while (RoutingTB_GetMode(i) != CLEAR)
        {
            if (RoutingTB_GetMode(i) == CONTAINER)
            {
                if (Luos_ReadFromContainer(service, RoutingTB_GetContainerID(i), &data_msg) == SUCCEED)
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
                    // Check if this is a message from pipe
                    if (data_msg->header.source == PipeLink_GetId())
                    {
                        do
                        {
                            // This message is a command from pipe
                            static char data_cmd[GATE_BUFF_SIZE];
                            // Convert the received data into Luos commands
                            if (Luos_ReceiveData(service, data_msg, data_cmd) == SUCCEED)
                            {
                                // We finish to receive this data, execute the received command
                                Convert_DataToLuos(service, data_cmd);
                            }
                        } while (Luos_ReadFromContainer(service, PipeLink_GetId(), &data_msg) == SUCCEED);
                        i++;
                        continue;
                    }
                    // get the source of this message
                    // Create container description
                    char *alias;
                    alias = RoutingTB_AliasFromId(data_msg->header.source);
                    if (alias != 0)
                    {
                        data_ok = true;
                        data_ptr += Convert_StartServiceData(data_ptr, alias);
                        // Convert all msgs from this container into data
                        do
                        {
                            data_ptr += Convert_MsgToData(data_msg, data_ptr);
                        } while (Luos_ReadFromContainer(service, data_msg->header.source, &data_msg) == SUCCEED);

                        data_ptr += Convert_EndServiceData(data_ptr);
                        LUOS_ASSERT((data_ptr - data) < GATE_BUFF_SIZE);
                    }
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
