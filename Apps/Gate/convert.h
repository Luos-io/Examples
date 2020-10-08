/*
 * convert.h
 *
 *  Created on: 20 juil. 2018
 *      Author: nicolasrabault
 */

#ifndef CONVERT_H_
#define CONVERT_H_

#include <json_mnger.h>
#include "cJSON.h"
#include "container_structs.h"
#include "luos.h"

void json_to_msg(container_t *container, uint16_t id, luos_type_t type, cJSON *jobj, msg_t *msg, char *data);
void msg_to_json(msg_t *msg, char *json);
void route_table_to_json(char *json);
void exclude_container_to_json(int id, char *json);

#endif /* CONVERT_H_ */
