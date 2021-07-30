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
#define POS_Pin       GPIO_PIN_0
#define POS_GPIO_Port GPIOA

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile angular_position_t angle = 0.0;

/*******************************************************************************
 * Function
 ******************************************************************************/
static void Potentiometer_MsgHandler(service_t *service, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Potentiometer_Init(void)
{
    revision_t revision = {.unmap = REV};
    // ******************* Analog measurement *******************
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    ADC_ChannelConfTypeDef sConfig   = {0};

    // Configure analog input pin channel
    GPIO_InitStruct.Pin  = POS_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(POS_GPIO_Port, &GPIO_InitStruct);

    // Enable  ADC clocks
    __HAL_RCC_ADC1_CLK_ENABLE();
    // Setup Adc to loop on DMA continuously
    Potentiometer_adc.Instance                   = ADC1;
    Potentiometer_adc.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;
    Potentiometer_adc.Init.Resolution            = ADC_RESOLUTION_12B;
    Potentiometer_adc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    Potentiometer_adc.Init.ScanConvMode          = ADC_SCAN_ENABLE;
    Potentiometer_adc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    Potentiometer_adc.Init.LowPowerAutoWait      = DISABLE;
    Potentiometer_adc.Init.LowPowerAutoPowerOff  = DISABLE;
    Potentiometer_adc.Init.ContinuousConvMode    = ENABLE;
    Potentiometer_adc.Init.DiscontinuousConvMode = DISABLE;
    Potentiometer_adc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    Potentiometer_adc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    Potentiometer_adc.Init.DMAContinuousRequests = ENABLE;
    Potentiometer_adc.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    if (HAL_ADC_Init(&Potentiometer_adc) != HAL_OK)
    {
        Error_Handler();
    }

    // Add ADC channel adc configuration.
    sConfig.Channel      = ADC_CHANNEL_0;
    sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&Potentiometer_adc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    // Enable DMA1 clock
    __HAL_RCC_DMA1_CLK_ENABLE();
    /* ADC1 DMA Init */
    /* ADC Init */
    Potentiometer_dma_adc.Instance                 = DMA1_Channel1;
    Potentiometer_dma_adc.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    Potentiometer_dma_adc.Init.PeriphInc           = DMA_PINC_DISABLE;
    Potentiometer_dma_adc.Init.MemInc              = DMA_MINC_ENABLE;
    Potentiometer_dma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    Potentiometer_dma_adc.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    Potentiometer_dma_adc.Init.Mode                = DMA_CIRCULAR;
    Potentiometer_dma_adc.Init.Priority            = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&Potentiometer_dma_adc) != HAL_OK)
    {
        Error_Handler();
    }
    // relinik DMA
    __HAL_LINKDMA(&Potentiometer_adc, DMA_Handle, Potentiometer_dma_adc);

    // disable DMA Irq
    HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    // Start infinite ADC measurement
    // Restart DMA
    HAL_ADC_Start_DMA(&Potentiometer_adc, (uint32_t *)analog_input.unmap, sizeof(analog_input.unmap) / sizeof(uint32_t));

    // ******************* service creation *******************
    Luos_CreateService(Potentiometer_MsgHandler, ANGLE_TYPE, "potentiometer", revision);
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
 * @brief Msg Handler call back when a msg receive for this service
 * @param Service destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Potentiometer_MsgHandler(service_t *service, msg_t *msg)
{
    if (msg->header.cmd == GET_CMD)
    {
        msg_t pub_msg;
        // fill the message infos
        pub_msg.header.target_mode = ID;
        pub_msg.header.target      = msg->header.source;
        AngularOD_PositionToMsg((angular_position_t *)&angle, &pub_msg);
        Luos_SendMsg(service, &pub_msg);
        return;
    }
}
