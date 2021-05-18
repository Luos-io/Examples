#include "luos_to_json.h"
#include "convert.h"
#include "json_alloc.h"
#include <stdio.h>
#include <stdbool.h>

static time_luos_t update_time = DEFAULT_REFRESH_S;

static void format_data(container_t *container, char *json);

void luos_to_json(container_t *container, char *json)
{
#ifdef GATE_POLLING
    collect_data(container);
#endif
    format_data(container, json);
}

//******************* sensor update ****************************
// This function will gather data from sensors and create a json string for you
void collect_data(container_t *container)
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
            Luos_SendMsg(container, &update_msg);
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
            Luos_SendMsg(container, &update_msg);
#endif
        }
    }
    // wait a little bit for the first reply
    uint32_t start_time = Luos_GetSystick();
    while ((start_time == Luos_GetSystick()) && (Luos_NbrAvailableMsg() == 0))
        ;
}

// This function will create a json string for containers datas
void format_data(container_t *container, char *json)
{
    char *json_ptr  = json;
    msg_t *json_msg = 0;
    uint8_t json_ok = false;
    if ((Luos_NbrAvailableMsg() > 0))
    {
        // Init the json string
        sprintf(json, "{\"containers\":{");
        json_ptr += strlen(json_ptr);
        // loop into containers.
        // get the oldest message
        while (Luos_ReadMsg(container, &json_msg) == SUCCEED)
        {
            // check if this is an assert
            if (json_msg->header.cmd == ASSERT)
            {
                char backup_json[strlen(json)];
                memcpy(backup_json, json, strlen(json));
                luos_assert_t assertion;
                memcpy(assertion.unmap, json_msg->data, json_msg->header.size);
                assertion.unmap[json_msg->header.size] = '\0';
                sprintf(json, "{\"assert\":{\"node_id\":%d,\"file\":\"%s\",\"line\":%d}}\n", json_msg->header.source, assertion.file, (unsigned int)assertion.line);
                json = json_alloc_set_tx_task(strlen(json));
                memcpy(json, backup_json, strlen(backup_json));
                continue;
            }
            // get the source of this message
            // Create container description
            char *alias;
            alias = RoutingTB_AliasFromId(json_msg->header.source);
            if (alias != 0)
            {
                json_ok = true;
                sprintf(json_ptr, "\"%s\":{", alias);
                json_ptr += strlen(json_ptr);
                // now add json data from container
                msg_to_json(json_msg, json_ptr);
                json_ptr += strlen(json_ptr);
                // Check if we receive other messages from this container
                while (Luos_ReadFromContainer(container, json_msg->header.source, &json_msg) == SUCCEED)
                {
                    // we receive some, add it to the Json
                    msg_to_json(json_msg, json_ptr);
                    json_ptr += strlen(json_ptr);
                }
                if (*json_ptr != '{')
                {
                    // remove the last "," char
                    *(--json_ptr) = '\0';
                }
                // End the container section
                sprintf(json_ptr, "},");
                json_ptr += strlen(json_ptr);
                LUOS_ASSERT((json_ptr - json) < JSON_BUFF_SIZE);
            }
        }
        if (json_ok)
        {
            // remove the last "," char
            *(--json_ptr) = '\0';
            // End the Json message
            sprintf(json_ptr, "}}\n");
            json = json_alloc_set_tx_task(strlen(json));
        }
        else
        {
            //create a void string
            *json = '\0';
        }
    }
    else
    {
        //create a void string
        *json = '\0';
    }
}

void set_update_time(time_luos_t new_time)
{
    update_time = new_time;
}

time_luos_t get_update_time(void)
{
    return update_time;
}