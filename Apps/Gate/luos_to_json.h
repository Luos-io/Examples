
#ifndef LUOS_TO_JSON
#define LUOS_TO_JSON

#include "luos.h"

void collect_data(container_t *service);
void luos_to_json(container_t *service);
void set_update_time(time_luos_t new_time);
time_luos_t get_update_time(void);

#endif /* LUOS_TO_JSON */
