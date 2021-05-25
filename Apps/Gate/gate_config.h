/******************************************************************************
 * @file gate_config
 * @brief Service gate configuration options
 * @author Luos
 ******************************************************************************/
#ifndef GATE_CONFIG_H
#define GATE_CONFIG_H

#include "luos.h"

#ifndef GATE_BUFF_SIZE
#define GATE_BUFF_SIZE 1024
#endif

#ifndef GATE_REFRESH_TIME_S
#define GATE_REFRESH_TIME_S 0.01f
#endif

// #define GATE_POLLING
// #define NODETECTION

#endif /* GATE_CONFIG_H */