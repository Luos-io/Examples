#include <stdio.h>
#include "json_mnger.h"
#include "cmd.h"
#include "convert.h"

static unsigned int delayms = 0;

//******************* sensor update ****************************
// This function will gather data from sensors and create a json string for you
void collect_data(module_t *module)
{
    msg_t json_msg;
    json_msg.header.target_mode = ID;
    json_msg.header.cmd = ASK_PUB_CMD;
    json_msg.header.size = 0;
    // ask modules to publish datas
    for (uint8_t i = 1; i <= RouteTB_GetLastModule(); i++)
    {
        // Check if this module is a sensor
        if (RouteTB_ModuleIsSensor(RouteTB_TypeFromID(i)))
        {
            // This module is a sensor so create a msg and send it
            json_msg.header.target = i;
            Luos_SendMsg(module, &json_msg);
        }
    }
}

// This function will create a json string for modules datas
void format_data(module_t *module, char *json)
{
    msg_t *json_msg = 0;
    if ((Luos_NbrAvailableMsg() > 0))
    {
        // Init the json string
        sprintf(json, "{\"modules\":{");
        // loop into modules.
        uint16_t i = 1;
        // get the oldest message
        while (Luos_ReadMsg(module, &json_msg) == SUCESS)
        {
            // get the source of this message
            i = json_msg->header.source;
            // Create module description
            char *alias;
            alias = RouteTB_AliasFromId(i);
            if (alias != 0)
            {
                sprintf(json, "%s\"%s\":{", json, alias);
                // now add json data from module
                msg_to_json(json_msg, &json[strlen(json)]);
                // Check if we receive other messages from this module
                while (Luos_ReadFromModule(module, i, &json_msg) == SUCESS)
                {
                    // we receive some, add it to the Json
                    msg_to_json(json_msg, &json[strlen(json)]);
                }
                if (json[strlen(json) - 1] != '{')
                {
                    // remove the last "," char
                    json[strlen(json) - 1] = '\0';
                }
                // End the module section
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

unsigned int get_delay(void)
{
    return delayms;
}

void set_delay(unsigned int new_delayms)
{
    delayms = new_delayms;
}
