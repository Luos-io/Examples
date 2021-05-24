#include "json_to_luos.h"
#include "convert.h"
#include <stdio.h>
#include "luos_to_json.h"
#include "gate_config.h"
#include "pipe_link.h"

// There is no stack here we use the latest command
volatile char detection_ask = 0;

void json_to_luos(container_t *service, char *json)
{
    msg_t msg;

    // check if we have a complete received command
    cJSON *root = cJSON_Parse(json);
    // check json integrity
    if (root == NULL)
    {
        // Error
        cJSON_Delete(root);
        return;
    }
    // check if this is a detection cmd
    if (cJSON_GetObjectItem(root, "detection") != NULL)
    {
        detection_ask++;
        cJSON_Delete(root);
        return;
    }
    if (cJSON_GetObjectItem(root, "baudrate") != NULL)
    {
        //create a message to setup the new baudrate
        cJSON *object = cJSON_GetObjectItem(root, "baudrate");
        if (cJSON_IsNumber(object))
        {
            uint32_t baudrate = (float)object->valueint;
            Luos_SendBaudrate(service, baudrate);
        }
        cJSON_Delete(root);
        return;
    }
    if (cJSON_GetObjectItem(root, "benchmark") != NULL)
    {
        //manage benchmark
        if (cJSON_IsObject(cJSON_GetObjectItem(root, "benchmark")))
        {
            // Get all parameters
            cJSON *parameters   = cJSON_GetObjectItem(root, "benchmark");
            uint32_t repetition = 0;
            if (cJSON_IsNumber(cJSON_GetObjectItem(parameters, "repetitions")))
            {
                repetition = (int)cJSON_GetObjectItem(parameters, "repetitions")->valueint;
            }
            uint32_t target_id = (int)cJSON_GetObjectItem(parameters, "target")->valueint;
            cJSON *item        = cJSON_GetObjectItem(parameters, "data");
            uint32_t size      = (int)cJSON_GetArrayItem(item, 0)->valueint;
            if (size > 0)
            {
                // find the first \r of the current buf
                int index = 0;
                for (index = 0; index < JSON_BUFF_SIZE; index++)
                {
                    if (json[index] == '\r')
                    {
                        index++;
                        break;
                    }
                }
                if (index < JSON_BUFF_SIZE - 1)
                {
                    // stop sensor polling during benchmark
                    set_update_time(0.0);
                    collect_data(service);
                    // create a message from parameters
                    msg.header.cmd         = REVISION;
                    msg.header.target_mode = IDACK;
                    msg.header.target      = target_id;
                    // save current time
                    uint32_t begin_systick = Luos_GetSystick();
                    uint32_t failed_msg_nb = 0;
                    // Before trying to send anything make sure to finish any transmission
                    while (Luos_TxComplete() == FAILED)
                        ;
                    // Wait 10ms allowing receiving messages to finish
                    uint32_t tickstart = Luos_GetSystick();
                    while ((Luos_GetSystick() - tickstart) < 10)
                        ;
                    // Flush every messages pending
                    Luos_Flush();
                    // To get the number of message failed we will use statistics
                    // We have to reinit the number of dropped message before start
                    uint8_t drop_back                                = service->node_statistics->memory.msg_drop_number;
                    service->node_statistics->memory.msg_drop_number = 0;
                    uint8_t retry_back                               = *service->ll_container->ll_stat.max_retry;
                    *service->ll_container->ll_stat.max_retry        = 0;
                    // send this message multiple time
                    int i = 0;
                    for (i = 0; i < repetition; i++)
                    {
                        Luos_SendData(service, &msg, &json[index], (unsigned int)size);
                    }
                    // Wait transmission end
                    while (Luos_TxComplete() == FAILED)
                        ;
                    // Get the number of failures on transmission
                    failed_msg_nb = service->node_statistics->memory.msg_drop_number;
                    // Get the number of retry
                    // If retry == max retry number consider all messages as lost
                    if (*service->ll_container->ll_stat.max_retry >= NBR_RETRY)
                    {
                        // We failed to transmit this message count all as failed
                        failed_msg_nb = repetition;
                    }
                    service->node_statistics->memory.msg_drop_number = drop_back;
                    *service->ll_container->ll_stat.max_retry        = retry_back;
                    uint32_t end_systick                             = Luos_GetSystick();
                    float data_rate                                  = (float)size * (float)(repetition - failed_msg_nb) / (((float)end_systick - (float)begin_systick) / 1000.0) * 8;
                    float fail_rate                                  = (float)failed_msg_nb * 100.0 / (float)repetition;
                    char tx_json[512];
                    sprintf(tx_json, "{\"benchmark\":{\"data_rate\":%.2f,\"fail_rate\":%.2f}}\n", data_rate, fail_rate);
                    send_to_pipe(service, tx_json, strlen(tx_json));
                    // restart sensor polling
                    set_update_time(DEFAULT_REFRESH_S);
                    collect_data(service);
                }
            }
        }
        cJSON_Delete(root);
        return;
    }
    cJSON *containers = cJSON_GetObjectItem(root, "containers");
    // Get containers
    if (cJSON_IsObject(containers))
    {
        // Loop into containers
        cJSON *container_jsn = containers->child;
        while (container_jsn != NULL)
        {
            // Create msg
            char *alias = container_jsn->string;
            uint16_t id = RoutingTB_IDFromAlias(alias);
            if (id == 65535)
            {
                // If alias doesn't exist in our list id_from_alias send us back -1 = 65535
                // So here there is an error in alias.
                cJSON_Delete(root);
                break;
            }
            luos_type_t type = RoutingTB_TypeFromID(id);
            json_to_msg(service, id, type, container_jsn, &msg, (char *)json);
            // Get next container
            container_jsn = container_jsn->next;
        }
        cJSON_Delete(root);
        return;
    }
}
