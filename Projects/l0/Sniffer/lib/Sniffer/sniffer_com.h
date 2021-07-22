/******************************************************************************
 * @file sniffer_com
 * @brief communication driver
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef SNIFFER_COM_H
#define SNIFFER_COM_H

#include "sniffer.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_ll_usart.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_dma.h"
#include "stm32f0xx_ll_system.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define SNIFFER_TX_CLK() __HAL_RCC_GPIOB_CLK_ENABLE();
#define SNIFFER_TX_PIN   GPIO_PIN_10
#define SNIFFER_TX_PORT  GPIOB
#define SNIFFER_TX_AF    GPIO_AF4_USART3

#define SNIFFER_RX_CLK() __HAL_RCC_GPIOB_CLK_ENABLE();
#define SNIFFER_RX_PIN   GPIO_PIN_11
#define SNIFFER_RX_PORT  GPIOB
#define SNIFFER_RX_AF    GPIO_AF4_USART3

#define SNIFFER_COM_CLOCK_ENABLE() __HAL_RCC_USART3_CLK_ENABLE()
#define SNIFFER_COM                USART3
#define SNIFFER_COM_IRQ            USART3_4_IRQn
#define SNIFFER_COM_IRQHANDLER()   USART3_4_IRQHandler()

#define SNIFFER_RCV_DMA_CLOCK_ENABLE() __HAL_RCC_DMA1_CLK_ENABLE()
#define SNIFFER_RCV_DMA                DMA1
#define SNIFFER_RCV_DMA_CHANNEL        LL_DMA_CHANNEL_6

#define SNIFFER_SEND_DMA_CLOCK_ENABLE()             __HAL_RCC_DMA1_CLK_ENABLE()
#define SNIFFER_SEND_DMA                            DMA1
#define SNIFFER_SEND_DMA_CHANNEL                    LL_DMA_CHANNEL_7
#define SNIFFER_SEND_DMA_TC(SNIFFER_SEND_DMA)       LL_DMA_IsActiveFlag_TC7(SNIFFER_SEND_DMA)
#define SNIFFER_SEND_DMA_CLEAR_TC(SNIFFER_SEND_DMA) LL_DMA_ClearFlag_TC7(SNIFFER_SEND_DMA)
#define SNIFFER_SEND_DMA_IRQ                        DMA1_Channel4_5_6_7_IRQn
#define SNIFFER_USART_RMP_DMA                       LL_SYSCFG_USART3_RMP_DMA1CH67
#define SNIFFER_SEND_DMA_IRQHANDLER()               DMA1_Channel4_5_6_7_IRQHandler()
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Functions
 ******************************************************************************/
void SnifferCom_Init(void);
void SnifferCom_Send(uint8_t *data, uint16_t size);
uint8_t SnifferCom_Pending(void);

#endif /*SNIFFER_COM_H*/
