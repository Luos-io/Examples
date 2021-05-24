#include "luos_to_json.h"
#include "convert.h"
#include <stdio.h>
#include <stdbool.h>
#include "gate_config.h"
#include "json_to_luos.h"
#include "pipe_link.h"

static time_luos_t update_time = DEFAULT_REFRESH_S;

static void format_data(container_t *service);

void luos_to_json(container_t *service)
{
    // If there is a dead container, manage it.
    exclude_container_to_json(service);
#ifdef GATE_POLLING
    collect_data(service);
#endif
    format_data(service);
}

//******************* sensor update ****************************
// This function will gather data from sensors and create a json string for you
void collect_data(container_t *service)
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
            Luos_SendMsg(service, &update_msg);
#endif
        }
    }
    // wait a little bit for the first reply
    uint32_t start_time = Luos_GetSystick();
    while ((start_time == Luos_GetSystick()) && (Luos_NbrAvailableMsg() == 0))
        ;
}

// This function will create a json string for containers datas
void format_data(container_t *service)
{
    uint32_t FirstNoReceptionDate = 0;
    char json[JSON_BUFF_SIZE];
    char *json_ptr  = json;
    msg_t *json_msg = 0;
    uint8_t json_ok = false;
    if ((Luos_NbrAvailableMsg() > 0))
    {
        // Init the json string
        memcpy(json, "{\"containers\":{", sizeof("{\"containers\":{"));
        json_ptr += sizeof("{\"containers\":{") - 1;
        // loop into containers.
        // get the oldest message
        while (Luos_ReadMsg(service, &json_msg) == SUCCEED)
        {
            // check if this is an assert
            if (json_msg->header.cmd == ASSERT)
            {
                char assert_json[512];
                luos_assert_t assertion;
                memcpy(assertion.unmap, json_msg->data, json_msg->header.size);
                assertion.unmap[json_msg->header.size] = '\0';
                sprintf(assert_json, "{\"assert\":{\"node_id\":%d,\"file\":\"%s\",\"line\":%d}}\n", json_msg->header.source, assertion.file, (unsigned int)assertion.line);
                // Send the message to pipe
                send_to_pipe(service, assert_json, strlen(assert_json));
                continue;
            }
            // Check if this is a message from pipe
            if (json_msg->header.source == get_pipe_id())
            {
                // This message is a command from pipe
                // Convert the received data into Luos commands
                static char json_cmd[JSON_BUFF_SIZE];
                if (Luos_ReceiveData(service, json_msg, json_cmd) == SUCCEED)
                {
                    // We finish to receive this data, execute the received command
                    json_to_luos(service, json_cmd);
                }
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
                // now add json data from this container
                do
                {
                    msg_to_json(json_msg, json_ptr);
                    json_ptr += strlen(json_ptr);
                } while (Luos_ReadFromContainer(service, json_msg->header.source, &json_msg) == SUCCEED);
                if (*json_ptr != '{')
                {
                    // remove the last "," char
                    *(--json_ptr) = '\0';
                }
                // End the container section
                memcpy(json_ptr, "},", sizeof("},"));
                json_ptr += sizeof("},") - 1;
                LUOS_ASSERT((json_ptr - json) < JSON_BUFF_SIZE);
            }
        }
        if (json_ok)
        {
            // remove the last "," char
            *(--json_ptr) = '\0';
            // End the Json message
            memcpy(json_ptr, "}}\n", sizeof("}}\n"));
            json_ptr += sizeof("}}\n") - 1;
            // Send the message to pipe
            send_to_pipe(service, json, json_ptr - json);
            FirstNoReceptionDate = 0;
        }
        else
        {
            // We don't receive anything.
            // After 1s void reception send void Json allowing client to send commands (because client could be synchronized to reception).
            if (FirstNoReceptionDate == 0)
            {
                FirstNoReceptionDate = Luos_GetSystick();
            }
            else if (Luos_GetSystick() - FirstNoReceptionDate > 1000)
            {
                sprintf(json, "{}\n");
                memcpy(json, "{}\n", sizeof("{}\n"));
                send_to_pipe(service, json, sizeof("{}\n"));
            }
        }
    }
    else
    {
        // We don't receive anything.
        // After 1s void reception send void Json allowing client to send commands (because client could be synchronized to reception).
        if (FirstNoReceptionDate == 0)
        {
            FirstNoReceptionDate = Luos_GetSystick();
        }
        else if (Luos_GetSystick() - FirstNoReceptionDate > 1000)
        {
            sprintf(json, "{}\n");
            memcpy(json, "{}\n", sizeof("{}\n"));
            send_to_pipe(service, json, sizeof("{}\n"));
        }
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