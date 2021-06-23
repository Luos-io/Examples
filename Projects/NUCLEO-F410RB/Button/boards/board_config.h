/******************************************************************************
 * @file board_config.h
 * @brief This file allow you to preconfigure all the pin and hardware ressource,
 *        use by Luos to create the Luos Network.
 *        Check the Luos default configuration for the family you choose in the
 *        file LuosHal_Config.h and define here Hardware ressource to match your
 *        Design
 * @MCU Family STM32F4
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

/*******************************************************************************
 * MCU CONFIG
 ******************************************************************************/

/*******************************************************************************
 * PINOUT CONFIG
 ******************************************************************************/

/*******************************************************************************
 * COM CONFIG
 ******************************************************************************/

/*******************************************************************************
 * COM DMA CONFIG
 ******************************************************************************/

/*******************************************************************************
 * COM TIMEOUT CONFIG
 ******************************************************************************/
#define LUOS_TIMER_CLOCK_ENABLE() __HAL_RCC_TIM5_CLK_ENABLE()
#define LUOS_TIMER TIM5
#define LUOS_TIMER_IRQ TIM5_IRQn
#define LUOS_TIMER_IRQHANDLER() TIM5_IRQHandler()
/*******************************************************************************
 * FLASH CONFIG
 ******************************************************************************/
#define FLASH_SECTOR FLASH_SECTOR_4

#endif /* _BOARD_CONFIG_H_ */
