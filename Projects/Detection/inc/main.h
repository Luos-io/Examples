
#ifndef __MAIN_H
#define __MAIN_H

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <luosHAL.h>
#include "GPIO_Setup.h"
#include "stm32f0xx_it.h"
#include "stm32f0xx_hal.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define POWER_SENSOR_Pin GPIO_PIN_2
#define POWER_SENSOR_GPIO_Port GPIOA
#define LED_Pin GPIO_PIN_3
#define LED_GPIO_Port GPIOA
#define COM_LVL_DOWN_Pin GPIO_PIN_5
#define COM_LVL_DOWN_GPIO_Port GPIOA
#define COM_LVL_UP_Pin GPIO_PIN_6
#define COM_LVL_UP_GPIO_Port GPIOA
#define TX_Pin GPIO_PIN_10
#define TX_GPIO_Port GPIOB
#define RX_Pin GPIO_PIN_11
#define RX_GPIO_Port GPIOB
#define PTPB_Pin GPIO_PIN_13
#define PTPB_GPIO_Port GPIOB
#define RX_EN_Pin GPIO_PIN_14
#define RX_EN_GPIO_Port GPIOB
#define TX_EN_Pin GPIO_PIN_15
#define TX_EN_GPIO_Port GPIOB
#define PTPA_Pin GPIO_PIN_8
#define PTPA_GPIO_Port GPIOA
#define COM_TX_Pin GPIO_PIN_9
#define COM_TX_GPIO_Port GPIOA
#define COM_RX_Pin GPIO_PIN_10
#define COM_RX_GPIO_Port GPIOA
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void assert_failed(uint8_t *file, uint32_t line);

#endif /* __MAIN_H */
