#ifndef GATE_H
#define GATE_H

#include "luos.h"

void gate_init(void);
void gate_loop(void);
#ifdef USE_SERIAL
int serial_write(char *data, int len);
#endif
#endif /* GATE_H */
