
#ifndef LUOS_TO_JSON
#define LUOS_TO_JSON

#include "luos.h"

void collect_data(container_t *container);
void luos_to_json(container_t *container, char *json);
void set_update_time(time_luos_t new_time);

#endif /* LUOS_TO_JSON */
