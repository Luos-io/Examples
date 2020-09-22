/******************************************************************************
 * @file potentiometer
 * @brief driver example a simple potentiometer
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "main.h"
#include "potentiometer.h"
#include "analog.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
// Pin configuration
#define POS_Pin GPIO_PIN_0
#define POS_GPIO_Port GPIOA

#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile angular_position_t angle = 0.0;

/*******************************************************************************
 * Function
 ******************************************************************************/
static void Potentiometer_MsgHandler(module_t *module, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Potentiometer_Init(void)
{
    // ******************* Analog measurement *******************
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    ADC_ChannelConfTypeDef sConfig = {0};
    // Stop DMA
    HAL_ADC_Stop_DMA(&Potentiometer_adc);

    // Configure analog input pin channel
    GPIO_InitStruct.Pin = POS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(POS_GPIO_Port, &GPIO_InitStruct);

    // Add ADC channel to Luos adc configuration.
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&Potentiometer_adc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    // relinik DMA
    __HAL_LINKDMA(&Potentiometer_adc, DMA_Handle, Potentiometer_dma_adc);

    // Restart DMA
    HAL_ADC_Start_DMA(&Potentiometer_adc, (uint32_t *)analog_input.unmap, sizeof(analog_input.unmap) / sizeof(uint32_t));

    // ******************* module creation *******************
    Luos_CreateModule(Potentiometer_MsgHandler, ANGLE_MOD, "potentiometer_mod", STRINGIFY(VERSION));
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Potentiometer_Loop(void)
{
    angle = ((float)analog_input.pos / 4096.0) * 300.0;
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this module
 * @param Module destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Potentiometer_MsgHandler(module_t *module, msg_t *msg)
{
    if (msg->header.cmd == ASK_PUB_CMD)
    {
        msg_t pub_msg;
        // fill the message infos
        pub_msg.header.target_mode = ID;
        pub_msg.header.target = msg->header.source;
        AngularOD_PositionToMsg((angular_position_t *)&angle, &pub_msg);
        Luos_SendMsg(module, &pub_msg);
        return;
    }
}
