/******************************************************************************
 * @file gpio_dev
 * @brief driver example a simple gpio_dev
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include <gpio_dev.h>
#include "main.h"
#include "analog.h"
#include "template_state.h"
#include "template_voltage.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

// Pin configuration
#define P1_Pin       GPIO_PIN_0
#define P1_GPIO_Port GPIOA
#define P9_Pin       GPIO_PIN_1
#define P9_GPIO_Port GPIOA
#define P8_Pin       GPIO_PIN_0
#define P8_GPIO_Port GPIOB
#define P7_Pin       GPIO_PIN_1
#define P7_GPIO_Port GPIOB

enum
{
    P2,
    P3,
    P4,
    P5,
    P6,
    GPIO_NB
} gpio_enum_t;

enum
{
    P1,
    P7,
    P8,
    P9,
    ANALOG_NB
} analog_enum_t;
/*******************************************************************************
 * Variables
 ******************************************************************************/
template_state_t gpio_template[GPIO_NB];
profile_state_t *gpio[GPIO_NB];

template_voltage_t analog_template[ANALOG_NB];
profile_voltage_t *analog[ANALOG_NB];

/*******************************************************************************
 * Function
 ******************************************************************************/
static void rx_digit_write_cb(container_t *container, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void GpioDev_Init(void)
{
    revision_t revision = {.Major = 1, .Minor = 0, .Build = 0};
    // ******************* Analog measurement *******************
    // interesting tutorial about ADC : https://visualgdb.com/tutorials/arm/stm32/adc/
    ADC_ChannelConfTypeDef sConfig   = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // Enable  ADC Gpio clocks
    //__HAL_RCC_GPIOA_CLK_ENABLE(); => already enabled previously
    /**ADC GPIO Configuration
    */
    // Configure analog input pin channel
    GPIO_InitStruct.Pin  = P1_Pin | P9_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin  = P8_Pin | P7_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    // Enable  ADC clocks
    __HAL_RCC_ADC1_CLK_ENABLE();
    // Setup Adc to loop on DMA continuously
    GpioDev_adc.Instance                   = ADC1;
    GpioDev_adc.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;
    GpioDev_adc.Init.Resolution            = ADC_RESOLUTION_12B;
    GpioDev_adc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    GpioDev_adc.Init.ScanConvMode          = ADC_SCAN_ENABLE;
    GpioDev_adc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    GpioDev_adc.Init.LowPowerAutoWait      = DISABLE;
    GpioDev_adc.Init.LowPowerAutoPowerOff  = DISABLE;
    GpioDev_adc.Init.ContinuousConvMode    = ENABLE;
    GpioDev_adc.Init.DiscontinuousConvMode = DISABLE;
    GpioDev_adc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    GpioDev_adc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    GpioDev_adc.Init.DMAContinuousRequests = ENABLE;
    GpioDev_adc.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    if (HAL_ADC_Init(&GpioDev_adc) != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.Channel      = ADC_CHANNEL_0;
    sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&GpioDev_adc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfig.Channel      = ADC_CHANNEL_9;
    sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&GpioDev_adc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfig.Channel      = ADC_CHANNEL_8;
    sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&GpioDev_adc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfig.Channel      = ADC_CHANNEL_1;
    sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&GpioDev_adc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // Enable DMA1 clock
    __HAL_RCC_DMA1_CLK_ENABLE();
    /* ADC1 DMA Init */
    /* ADC Init */
    GpioDev_dma_adc.Instance                 = DMA1_Channel1;
    GpioDev_dma_adc.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    GpioDev_dma_adc.Init.PeriphInc           = DMA_PINC_DISABLE;
    GpioDev_dma_adc.Init.MemInc              = DMA_MINC_ENABLE;
    GpioDev_dma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    GpioDev_dma_adc.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    GpioDev_dma_adc.Init.Mode                = DMA_CIRCULAR;
    GpioDev_dma_adc.Init.Priority            = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&GpioDev_dma_adc) != HAL_OK)
    {
        Error_Handler();
    }
    __HAL_LINKDMA(&GpioDev_adc, DMA_Handle, GpioDev_dma_adc);
    // disable DMA Irq
    HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    // Start infinite ADC measurement
    HAL_ADC_Start_DMA(&GpioDev_adc, (uint32_t *)analog_input.unmap, sizeof(analog_input_t) / sizeof(uint32_t));
    // ************* Analog containers creation *******************
    // Link user varaibles to template profile.
    for (uint8_t i = 0; i < ANALOG_NB; i++)
    {
        analog[i] = &analog_template[i].profile;
    }
    // Profile configuration
    analog[P1]->access = READ_ONLY_ACCESS;
    analog[P7]->access = READ_ONLY_ACCESS;
    analog[P8]->access = READ_ONLY_ACCESS;
    analog[P9]->access = READ_ONLY_ACCESS;
    // Container creation following template
    TemplateVoltage_CreateContainer(0, &analog_template[P1], "analog_read_P1", revision);
    TemplateVoltage_CreateContainer(0, &analog_template[P7], "analog_read_P7", revision);
    TemplateVoltage_CreateContainer(0, &analog_template[P8], "analog_read_P8", revision);
    TemplateVoltage_CreateContainer(0, &analog_template[P9], "analog_read_P9", revision);

    // ************* Digital containers creation *******************
    // Link user varaibles to template profile.
    for (uint8_t i = 0; i < GPIO_NB; i++)
    {
        gpio[i] = &gpio_template[i].profile;
    }
    // Input profile configuration
    gpio[P5]->access = READ_ONLY_ACCESS;
    gpio[P6]->access = READ_ONLY_ACCESS;
    // Container creation following template
    TemplateState_CreateContainer(0, &gpio_template[P5], "digit_read_P5", revision);
    TemplateState_CreateContainer(0, &gpio_template[P6], "digit_read_P6", revision);

    // Output profile configuration
    gpio[P2]->access = WRITE_ONLY_ACCESS;
    gpio[P3]->access = WRITE_ONLY_ACCESS;
    gpio[P4]->access = WRITE_ONLY_ACCESS;
    // Container creation following template, for this one we one to use a target evnet using callback
    TemplateState_CreateContainer(rx_digit_write_cb, &gpio_template[P2], "digit_write_P2", revision);
    TemplateState_CreateContainer(rx_digit_write_cb, &gpio_template[P3], "digit_write_P3", revision);
    TemplateState_CreateContainer(rx_digit_write_cb, &gpio_template[P4], "digit_write_P4", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void GpioDev_Loop(void)
{
    // update gpio input
    gpio[P5]->state = (bool)(HAL_GPIO_ReadPin(P5_GPIO_Port, P5_Pin) > 0);
    gpio[P6]->state = (bool)(HAL_GPIO_ReadPin(P6_GPIO_Port, P6_Pin) > 0);

    // update analog measurement
    analog[P1]->voltage = ((float)analog_input.p1 / 4096.0f) * 3.3f;
    analog[P7]->voltage = ((float)analog_input.p7 / 4096.0f) * 3.3f;
    analog[P8]->voltage = ((float)analog_input.p8 / 4096.0f) * 3.3f;
    analog[P9]->voltage = ((float)analog_input.p9 / 4096.0f) * 3.3f;
}

static void rx_digit_write_cb(container_t *container, msg_t *msg)
{
    if (msg->header.cmd == IO_STATE)
    {
        // update pin state on event
        HAL_GPIO_WritePin(P2_GPIO_Port, P2_Pin, gpio[P2]->state);
        HAL_GPIO_WritePin(P3_GPIO_Port, P3_Pin, gpio[P3]->state);
        HAL_GPIO_WritePin(P4_GPIO_Port, P4_Pin, gpio[P4]->state);
    }
}
