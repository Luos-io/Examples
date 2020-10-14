/******************************************************************************
 * @file
 * Pin_Mux
 * @TS
 * @version 1.0.0
******************************************************************************/
#include "GPIO_Setup.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/

void GPIO_Setup(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    HAL_GPIO_WritePin(COM_LVL_UP_PORT, COM_LVL_UP_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(COM_LVL_DOWN_PORT, COM_LVL_DOWN_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, RX_EN_PIN | TX_EN_PIN, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(PTPA_PORT, PTPA_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(PTPB_PORT, PTPB_PIN, GPIO_PIN_RESET);

    /*Configure GPIO pin : COM_LVL_DOWN_PIN */
    GPIO_InitStruct.Pin = COM_LVL_DOWN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(COM_LVL_DOWN_PORT, &GPIO_InitStruct);

    /*Configure GPIO pin : COM_LVL_UP_PIN */
    GPIO_InitStruct.Pin = COM_LVL_UP_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(COM_LVL_UP_PORT, &GPIO_InitStruct);

    /*Configure GPIO pin : PTPB_PIN */
    GPIO_InitStruct.Pin = PTPB_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PTPB_PORT, &GPIO_InitStruct);

    /*Configure GPIO pin : PTPA_PIN */
    GPIO_InitStruct.Pin = PTPA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(PTPA_PORT, &GPIO_InitStruct);

    /*Configure GPIO pins : RX_EN_Pin TX_EN_Pin */
    GPIO_InitStruct.Pin = RX_EN_PIN | TX_EN_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /*Configure GPIO pin : TxPin */
    GPIO_InitStruct.Pin = COM_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
    HAL_GPIO_Init(COM_TX_PORT, &GPIO_InitStruct);

    /*Configure GPIO pin : RxPin */
    GPIO_InitStruct.Pin = COM_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF1_USART1;
    HAL_GPIO_Init(COM_RX_PORT, &GPIO_InitStruct);

    /* EXTI interrupt init*/
    HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

    //  GPIO_InitStruct.Pin = COM_TX_DETECT_Pin;
    //  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    //  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    //  GPIO_InitStruct.Pull = GPIO_PULLUP;
    //  HAL_GPIO_Init(COM_TX_DETECT_Port, &GPIO_InitStruct);

    //	//PC09
    //	GPIO_InitStruct.Pin = G_LED_PIN;
    //	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    //	GPIO_InitStruct.Pull = GPIO_PULLUP;
    //	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    //	HAL_GPIO_Init(G_LED_PORT, &GPIO_InitStruct);
    //
    //	//PC08
    //	GPIO_InitStruct.Pin = R_LED_PIN;
    //	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    //	GPIO_InitStruct.Pull = GPIO_PULLUP;
    //	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    //	HAL_GPIO_Init(R_LED_PORT, &GPIO_InitStruct);
    //
    //	//PA0
    //	GPIO_InitStruct.Pin = U_BUTTON_PIN;
    //	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    //	GPIO_InitStruct.Pull = GPIO_PULLUP;
    //	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    //	HAL_GPIO_Init(U_BUTTON_PORT, &GPIO_InitStruct);
}
