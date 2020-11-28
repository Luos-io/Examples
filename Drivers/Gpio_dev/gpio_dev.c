/******************************************************************************
 * @file gpio_dev
 * @brief driver example a simple gpio_dev
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include <gpio_dev.h>
#include "main.h"
#include "analog.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

// Pin configuration
#define P1_Pin GPIO_PIN_0
#define P1_GPIO_Port GPIOA
#define P9_Pin GPIO_PIN_1
#define P9_GPIO_Port GPIOA
#define P8_Pin GPIO_PIN_0
#define P8_GPIO_Port GPIOB
#define P7_Pin GPIO_PIN_1
#define P7_GPIO_Port GPIOB

enum
{
    P1,
    P2,
    P3,
    P4,
    P5,
    P6,
    P7,
    P8,
    P9
};
/*******************************************************************************
 * Variables
 ******************************************************************************/
container_t *pin[9];

/*******************************************************************************
 * Function
 ******************************************************************************/
static void rx_digit_write_cb(container_t *container, msg_t *msg);
static void rx_digit_read_cb(container_t *container, msg_t *msg);
static void rx_analog_read_cb(container_t *container, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void GpioDev_Init(void)
{
	revision_t revision = {.unmap = REV};
    // ******************* Analog measurement *******************
    // interesting tutorial about ADC : https://visualgdb.com/tutorials/arm/stm32/adc/
    ADC_ChannelConfTypeDef sConfig = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // Enable  ADC Gpio clocks
    //__HAL_RCC_GPIOA_CLK_ENABLE(); => already enabled previously
    /**ADC GPIO Configuration
    */
    // Configure analog input pin channel
    GPIO_InitStruct.Pin = P1_Pin | P9_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = P8_Pin | P7_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    // Enable  ADC clocks
    __HAL_RCC_ADC1_CLK_ENABLE();
    // Setup Adc to loop on DMA continuously
    GpioDev_adc.Instance = ADC1;
    GpioDev_adc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    GpioDev_adc.Init.Resolution = ADC_RESOLUTION_12B;
    GpioDev_adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    GpioDev_adc.Init.ScanConvMode = ADC_SCAN_ENABLE;
    GpioDev_adc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    GpioDev_adc.Init.LowPowerAutoWait = DISABLE;
    GpioDev_adc.Init.LowPowerAutoPowerOff = DISABLE;
    GpioDev_adc.Init.ContinuousConvMode = ENABLE;
    GpioDev_adc.Init.DiscontinuousConvMode = DISABLE;
    GpioDev_adc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    GpioDev_adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    GpioDev_adc.Init.DMAContinuousRequests = ENABLE;
    GpioDev_adc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
    if (HAL_ADC_Init(&GpioDev_adc) != HAL_OK)
    {
        Error_Handler();
    }

    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&GpioDev_adc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfig.Channel = ADC_CHANNEL_9;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&GpioDev_adc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfig.Channel = ADC_CHANNEL_8;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&GpioDev_adc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&GpioDev_adc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

    // Enable DMA1 clock
    __HAL_RCC_DMA1_CLK_ENABLE();
    /* ADC1 DMA Init */
    /* ADC Init */
    GpioDev_dma_adc.Instance = DMA1_Channel1;
    GpioDev_dma_adc.Init.Direction = DMA_PERIPH_TO_MEMORY;
    GpioDev_dma_adc.Init.PeriphInc = DMA_PINC_DISABLE;
    GpioDev_dma_adc.Init.MemInc = DMA_MINC_ENABLE;
    GpioDev_dma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    GpioDev_dma_adc.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    GpioDev_dma_adc.Init.Mode = DMA_CIRCULAR;
    GpioDev_dma_adc.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&GpioDev_dma_adc) != HAL_OK)
    {
        Error_Handler();
    }
    __HAL_LINKDMA(&GpioDev_adc, DMA_Handle, GpioDev_dma_adc);
    // disable DMA Irq
    HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    // Start infinite ADC measurement
    HAL_ADC_Start_DMA(&GpioDev_adc, (uint32_t *)analog_input.unmap, sizeof(analog_input_t) / sizeof(uint32_t));
    // ************* containers creation *******************
    pin[P1] = Luos_CreateContainer(rx_analog_read_cb, VOLTAGE_MOD, "analog_read_P1", revision);
    pin[P7] = Luos_CreateContainer(rx_analog_read_cb, VOLTAGE_MOD, "analog_read_P7", revision);
    pin[P8] = Luos_CreateContainer(rx_analog_read_cb, VOLTAGE_MOD, "analog_read_P8", revision);
    pin[P9] = Luos_CreateContainer(rx_analog_read_cb, VOLTAGE_MOD, "analog_read_P9", revision);
    pin[P5] = Luos_CreateContainer(rx_digit_read_cb, STATE_MOD, "digit_read_P5", revision);
    pin[P6] = Luos_CreateContainer(rx_digit_read_cb, STATE_MOD, "digit_read_P6", revision);
    pin[P2] = Luos_CreateContainer(rx_digit_write_cb, STATE_MOD, "digit_write_P2", revision);
    pin[P3] = Luos_CreateContainer(rx_digit_write_cb, STATE_MOD, "digit_write_P3", revision);
    pin[P4] = Luos_CreateContainer(rx_digit_write_cb, STATE_MOD, "digit_write_P4", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void GpioDev_Loop(void)
{
}

static void rx_digit_read_cb(container_t *container, msg_t *msg)
{
    if (msg->header.cmd == ASK_PUB_CMD)
    {
        msg_t pub_msg;
        // fill the message infos
        pub_msg.header.cmd = IO_STATE;
        pub_msg.header.target_mode = ID;
        pub_msg.header.target = msg->header.source;
        pub_msg.header.size = sizeof(char);

        if (container == pin[P5])
        {
            pub_msg.data[0] = (char)(HAL_GPIO_ReadPin(P5_GPIO_Port, P5_Pin) > 0);
        }
        else if (container == pin[P6])
        {
            pub_msg.data[0] = (char)(HAL_GPIO_ReadPin(P6_GPIO_Port, P6_Pin) > 0);
        }
        else
        {
            return;
        }
        Luos_SendMsg(container, &pub_msg);
        return;
    }
}

static void rx_digit_write_cb(container_t *container, msg_t *msg)
{
    if (msg->header.cmd == IO_STATE)
    {
        // we have to update pin state
        if (container == pin[P2])
        {
            HAL_GPIO_WritePin(P2_GPIO_Port, P2_Pin, msg->data[0]);
        }
        if (container == pin[P3])
        {
            HAL_GPIO_WritePin(P3_GPIO_Port, P3_Pin, msg->data[0]);
        }
        if (container == pin[P4])
        {
            HAL_GPIO_WritePin(P4_GPIO_Port, P4_Pin, msg->data[0]);
        }
    }
}

static void rx_analog_read_cb(container_t *container, msg_t *msg)
{
    if (msg->header.cmd == ASK_PUB_CMD)
    {
        msg_t pub_msg;
        voltage_t volt;
        if (container == pin[P1])
        {
            volt = ((float)analog_input.p1 / 4096.0f) * 3.3f;
        }
        else if (container == pin[P7])
        {
            volt = ((float)analog_input.p7 / 4096.0f) * 3.3f;
        }
        else if (container == pin[P8])
        {
            volt = ((float)analog_input.p8 / 4096.0f) * 3.3f;
        }
        else if (container == pin[P9])
        {
            volt = ((float)analog_input.p9 / 4096.0f) * 3.3f;
        }
        else
        {
            return;
        }
        // fill the message infos
        pub_msg.header.target_mode = ID;
        pub_msg.header.target = msg->header.source;
        ElectricOD_VoltageToMsg(&volt, &pub_msg);
        Luos_SendMsg(container, &pub_msg);
        return;
    }
}
