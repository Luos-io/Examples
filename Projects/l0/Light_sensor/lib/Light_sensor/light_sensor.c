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
#define LIGHT_Pin       GPIO_PIN_1
#define LIGHT_GPIO_Port GPIOA

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile illuminance_t lux = 0.0;

/*******************************************************************************
 * Function
 ******************************************************************************/
static void LightSensor_MsgHandler(container_t *container, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void LightSensor_Init(void)
{
    revision_t revision = {.unmap = REV};
    // ******************* Analog measurement *******************
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    ADC_ChannelConfTypeDef sConfig   = {0};

    // Configure analog input pin channel
    GPIO_InitStruct.Pin  = LIGHT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(LIGHT_GPIO_Port, &GPIO_InitStruct);

    // Enable  ADC clocks
    __HAL_RCC_ADC1_CLK_ENABLE();
    // Setup Adc to loop on DMA continuously
    LightSensor_adc.Instance                   = ADC1;
    LightSensor_adc.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;
    LightSensor_adc.Init.Resolution            = ADC_RESOLUTION_12B;
    LightSensor_adc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    LightSensor_adc.Init.ScanConvMode          = ADC_SCAN_ENABLE;
    LightSensor_adc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    LightSensor_adc.Init.LowPowerAutoWait      = DISABLE;
    LightSensor_adc.Init.LowPowerAutoPowerOff  = DISABLE;
    LightSensor_adc.Init.ContinuousConvMode    = ENABLE;
    LightSensor_adc.Init.DiscontinuousConvMode = DISABLE;
    LightSensor_adc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    LightSensor_adc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    LightSensor_adc.Init.DMAContinuousRequests = ENABLE;
    LightSensor_adc.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    if (HAL_ADC_Init(&LightSensor_adc) != HAL_OK)
    {
        Error_Handler();
    }

    // Add ADC channel to Luos adc configuration.
    sConfig.Channel      = ADC_CHANNEL_1;
    sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&LightSensor_adc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    // Enable DMA1 clock
    __HAL_RCC_DMA1_CLK_ENABLE();
    /* ADC1 DMA Init */
    /* ADC Init */
    LightSensor_dma_adc.Instance                 = DMA1_Channel1;
    LightSensor_dma_adc.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    LightSensor_dma_adc.Init.PeriphInc           = DMA_PINC_DISABLE;
    LightSensor_dma_adc.Init.MemInc              = DMA_MINC_ENABLE;
    LightSensor_dma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    LightSensor_dma_adc.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    LightSensor_dma_adc.Init.Mode                = DMA_CIRCULAR;
    LightSensor_dma_adc.Init.Priority            = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&LightSensor_dma_adc) != HAL_OK)
    {
        Error_Handler();
    }
    // relinik DMA
    __HAL_LINKDMA(&LightSensor_adc, DMA_Handle, LightSensor_dma_adc);

    // disable DMA Irq
    HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    // Start infinite ADC measurement
    // Restart DMA
    HAL_ADC_Start_DMA(&LightSensor_adc, (uint32_t *)analog_input.unmap, sizeof(analog_input.unmap) / sizeof(uint32_t));

    // ******************* Analog measurement *******************

    // ******************* container creation *******************
    Luos_CreateContainer(LightSensor_MsgHandler, LIGHT_TYPE, "light_sensor_mod", revision);
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
 * @brief Msg Handler call back when a msg receive for this container
 * @param Container destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void LightSensor_MsgHandler(container_t *container, msg_t *msg)
{
    if (msg->header.cmd == GET_CMD)
    {
        msg_t pub_msg;
        // fill the message infos
        pub_msg.header.target_mode = ID;
        pub_msg.header.target      = msg->header.source;
        IlluminanceOD_IlluminanceToMsg((illuminance_t *)&lux, &pub_msg);
        Luos_SendMsg(container, &pub_msg);
        return;
    }
}
