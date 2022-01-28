/******************************************************************************
 * @file led_com
 * @brief communication driver
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef LED_DRV_H
#define LED_DRV_H

#include "luos.h"
#include "stm32f0xx_hal.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_system.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define LED_PIN  GPIO_PIN_5
#define LED_PORT GPIOA

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/

void LedDrv_Init();
void LedDrv_Write(uint8_t value);

#endif /* LED_DRV_H */