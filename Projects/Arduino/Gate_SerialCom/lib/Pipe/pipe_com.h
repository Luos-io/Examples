/******************************************************************************
 * @file pipe_com
 * @brief communication driver
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef PIPE_COM_H
#define PIPE_COM_H

#include "pipe_buffer.h"
#include "streaming.h"

#include "stm32f0xx_hal.h"
#include "stm32f0xx_ll_usart.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_hal.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void PipeCom_Init(void);
void PipeCom_SetP2L(uint8_t *data, uint16_t size);
void PipeCom_SendL2P(uint8_t *data, uint16_t size);
volatile uint8_t PipeCom_SendL2PPending(void);
streaming_channel_t *get_L2P_StreamChannel(void);
streaming_channel_t *get_P2L_StreamChannel(void);

#endif /* PIPE_H */
