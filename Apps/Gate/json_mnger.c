#include <stdio.h>
#include "json_mnger.h"
#include "cmd.h"
#include "convert.h"
#include "gate.h"
#include "luos.h"

static time_luos_t update_time = DEFAULT_REFRESH_MS;

//******************* sensor update ****************************
// This function will gather data from sensors and create a json string for you
void collect_data(container_t *container)
{
    msg_t update_msg;
    update_msg.header.target_mode = IDACK;
    update_msg.header.cmd = ASK_PUB_CMD;
    // ask containers to publish datas
    for (uint8_t i = 1; i <= RoutingTB_GetLastContainer(); i++)
    {
        // Check if this container is a sensor
        if (RoutingTB_ContainerIsSensor(RoutingTB_TypeFromID(i)))
        {
            // This contaiiner is a sensor so create a msg to enable auto update
            update_msg.header.target = i;
            time_to_msg(&update_time, &update_msg);
            update_msg.header.cmd = UPDATE_PUB;
            Luos_SendMsg(module, &update_msg);
        }
    }
}

// This function will create a json string for containers datas
void format_data(container_t *container, char *json)
{
    msg_t *json_msg = 0;
    if ((Luos_NbrAvailableMsg() > 0))
    {
        // Init the json string
        sprintf(json, "{\"containers\":{");
        // loop into containers.
        uint16_t i = 1;
        // get the oldest message
        while (Luos_ReadMsg(container, &json_msg) == SUCCEED)
        {
            // get the source of this message
            i = json_msg->header.source;
            // Create container description
            char *alias;
            alias = RoutingTB_AliasFromId(i);
            if (alias != 0)
            {
                sprintf(json, "%s\"%s\":{", json, alias);
                // now add json data from container
                msg_to_json(json_msg, &json[strlen(json)]);
                // Check if we receive other messages from this container
                while (Luos_ReadFromContainer(container, i, &json_msg) == SUCCEED)
                {
                    // we receive some, add it to the Json
                    msg_to_json(json_msg, &json[strlen(json)]);
                }
                if (json[strlen(json) - 1] != '{')
                {
                    // remove the last "," char
                    json[strlen(json) - 1] = '\0';
                }
                // End the container section
                sprintf(json, "%s},", json);
            }
        }
        // remove the last "," char
        json[strlen(json) - 1] = '\0';
        // End the Json message
        sprintf(json, "%s}}\n", json);
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
