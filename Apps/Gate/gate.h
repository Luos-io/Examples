/******************************************************************************
 * @file gate
 * @brief Module gate
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef GATE_H
#define GATE_H

#include "luos.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void Gate_Init(void);
void Gate_Loop(void);
#ifdef USE_SERIAL
int serial_write(char *data, int len);
#endif
#endif /* GATE_H */
