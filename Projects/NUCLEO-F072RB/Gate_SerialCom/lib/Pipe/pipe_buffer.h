/******************************************************************************
 * @file pipe_com
 * @brief communication driver
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef PIPE_BUFFER_H
#define PIPE_BUFFER_H

#include <stdint.h>
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifndef PIPE_TO_LUOS_BUFFER_SIZE
#define PIPE_TO_LUOS_BUFFER_SIZE 1024
#endif
#ifndef LUOS_TO_PIPE_BUFFER_SIZE
#define LUOS_TO_PIPE_BUFFER_SIZE 2048
#endif

#define SERIAL_HEADER 0x7E
#define SERIAL_FOOTER 0x81
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void PipeBuffer_Init(void);
uint8_t PipeBuffer_GetP2LTask(uint16_t *size);
uint8_t *PipeBuffer_GetP2LBuffer(void);
uint8_t *PipeBuffer_GetL2PBuffer(void);

#endif /* PIPE_BUFFER_H */
