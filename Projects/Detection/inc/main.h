
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
#define ROBUS_POWER_SENSOR_Pin GPIO_PIN_2
#define ROBUS_POWER_SENSOR_GPIO_Port GPIOA
#define LED_Pin GPIO_PIN_3
#define LED_GPIO_Port GPIOA
#define RS485_LVL_DOWN_Pin GPIO_PIN_5
#define RS485_LVL_DOWN_GPIO_Port GPIOA
#define RS485_LVL_UP_Pin GPIO_PIN_6
#define RS485_LVL_UP_GPIO_Port GPIOA
#define TX_Pin GPIO_PIN_10
#define TX_GPIO_Port GPIOB
#define RX_Pin GPIO_PIN_11
#define RX_GPIO_Port GPIOB
#define ROBUS_PTPB_Pin GPIO_PIN_13
#define ROBUS_PTPB_GPIO_Port GPIOB
#define ROBUS_RE_Pin GPIO_PIN_14
#define ROBUS_RE_GPIO_Port GPIOB
#define ROBUS_DE_Pin GPIO_PIN_15
#define ROBUS_DE_GPIO_Port GPIOB
#define ROBUS_PTPA_Pin GPIO_PIN_8
#define ROBUS_PTPA_GPIO_Port GPIOA
#define ROBUS_TX_Pin GPIO_PIN_9
#define ROBUS_TX_GPIO_Port GPIOA
#define ROBUS_RX_Pin GPIO_PIN_10
#define ROBUS_RX_GPIO_Port GPIOA
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line);

#endif /* __MAIN_H */


