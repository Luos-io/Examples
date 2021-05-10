
#ifndef ROUTINGTABLE
#define ROUTINGTABLE

#include "luos_list.h"
#include "luos.h"
#include "cmd.h"
#include "convert.h"

void collect_data(container_t *container);
void format_data(container_t *container, char *json);
unsigned int get_delay(void);
void set_delay(unsigned int new_delayms);

#endif /* ROUTINGTABLE */
