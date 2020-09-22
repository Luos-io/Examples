/******************************************************************************
 * @file light sensor
 * @brief driver example a simple light sensor
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "main.h"
#include "light_sensor.h"
#include "analog.h"
#include "string.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
// Pin configuration
#define LIGHT_Pin GPIO_PIN_1
#define LIGHT_GPIO_Port GPIOA

#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s
/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile illuminance_t lux = 0.0;

/*******************************************************************************
 * Function
 ******************************************************************************/
static void LightSensor_MsgHandler(module_t *module, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void LightSensor_Init(void)
{
    // ******************* Analog measurement *******************
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    ADC_ChannelConfTypeDef sConfig = {0};
    // Stop DMA
    HAL_ADC_Stop_DMA(&LightSensor_adc);

    // Configure analog input pin channel
    GPIO_InitStruct.Pin = LIGHT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LIGHT_GPIO_Port, &GPIO_InitStruct);

    // Add ADC channel to Luos adc configuration.
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    while (HAL_ADC_ConfigChannel(&LightSensor_adc, &sConfig) != HAL_OK)
        ;
    // relinik DMA
    __HAL_LINKDMA(&LightSensor_adc, DMA_Handle, LightSensor_dma_adc);

    // Restart DMA
    HAL_ADC_Start_DMA(&LightSensor_adc, (uint32_t *)analog_input.unmap, sizeof(analog_input.unmap) / sizeof(uint32_t));

    // ******************* module creation *******************
    Luos_CreateModule(LightSensor_MsgHandler, LIGHT_MOD, "light_sensor_mod", STRINGIFY(VERSION));
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void LightSensor_Loop(void)
{
    lux = (((float)analog_input.light / 4096.0f) * 3.3f) * 1000.0f;
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this module
 * @param Module destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void LightSensor_MsgHandler(module_t *module, msg_t *msg)
{
    if (msg->header.cmd == ASK_PUB_CMD)
    {
        msg_t pub_msg;
        // fill the message infos
        pub_msg.header.target_mode = ID;
        pub_msg.header.target = msg->header.source;
        IlluminanceOD_IlluminanceToMsg((illuminance_t *)&lux, &pub_msg);
        Luos_SendMsg(module, &pub_msg);
        return;
    }
}
