#ifndef CMD_H
#define CMD_H

#include "luos.h"
#include "stdint.h"

extern volatile char detection_ask;

void check_json(uint16_t carac_nbr);
void send_cmds(container_t *container);

#endif /* CMD_H */
