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
#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

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
module_t *pin[9];

/*******************************************************************************
 * Function
 ******************************************************************************/
static void rx_digit_write_cb(module_t *module, msg_t *msg);
static void rx_digit_read_cb(module_t *module, msg_t *msg);
static void rx_analog_read_cb(module_t *module, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void GpioDev_Init(void)
{
    // ******************* Analog measurement *******************
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    ADC_ChannelConfTypeDef sConfig = {0};
    // Stop DMA
    HAL_ADC_Stop_DMA(&GpioDev_adc);

    // Configure analog input pin channel
    GPIO_InitStruct.Pin = P1_Pin | P9_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = P8_Pin | P7_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Add ADC channel to Luos adc configuration.
    sConfig.Channel = ADC_CHANNEL_0;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    while (HAL_ADC_ConfigChannel(&GpioDev_adc, &sConfig) != HAL_OK)
        ;
    sConfig.Channel = ADC_CHANNEL_9;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    while (HAL_ADC_ConfigChannel(&GpioDev_adc, &sConfig) != HAL_OK)
        ;
    sConfig.Channel = ADC_CHANNEL_8;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    while (HAL_ADC_ConfigChannel(&GpioDev_adc, &sConfig) != HAL_OK)
        ;
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    while (HAL_ADC_ConfigChannel(&GpioDev_adc, &sConfig) != HAL_OK)
        ;

    // relinik DMA
    __HAL_LINKDMA(&GpioDev_adc, DMA_Handle, GpioDev_dma_adc);

    // Restart DMA
    HAL_ADC_Start_DMA(&GpioDev_adc, (uint32_t *)analog_input.unmap, sizeof(analog_input.unmap) / sizeof(uint32_t));
    // ************* modules creation *******************
    pin[P1] = Luos_CreateModule(rx_analog_read_cb, VOLTAGE_MOD, "analog_read_P1", STRINGIFY(VERSION));
    pin[P7] = Luos_CreateModule(rx_analog_read_cb, VOLTAGE_MOD, "analog_read_P7", STRINGIFY(VERSION));
    pin[P8] = Luos_CreateModule(rx_analog_read_cb, VOLTAGE_MOD, "analog_read_P8", STRINGIFY(VERSION));
    pin[P9] = Luos_CreateModule(rx_analog_read_cb, VOLTAGE_MOD, "analog_read_P9", STRINGIFY(VERSION));
    pin[P5] = Luos_CreateModule(rx_digit_read_cb, STATE_MOD, "digit_read_P5", STRINGIFY(VERSION));
    pin[P6] = Luos_CreateModule(rx_digit_read_cb, STATE_MOD, "digit_read_P6", STRINGIFY(VERSION));
    pin[P2] = Luos_CreateModule(rx_digit_write_cb, STATE_MOD, "digit_write_P2", STRINGIFY(VERSION));
    pin[P3] = Luos_CreateModule(rx_digit_write_cb, STATE_MOD, "digit_write_P3", STRINGIFY(VERSION));
    pin[P4] = Luos_CreateModule(rx_digit_write_cb, STATE_MOD, "digit_write_P4", STRINGIFY(VERSION));
    Luos_ModuleEnableRT(pin[P2]);
    Luos_ModuleEnableRT(pin[P3]);
    Luos_ModuleEnableRT(pin[P4]);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void GpioDev_Loop(void)
{
}

static void rx_digit_read_cb(module_t *module, msg_t *msg)
{
    if (msg->header.cmd == ASK_PUB_CMD)
    {
        msg_t pub_msg;
        // fill the message infos
        pub_msg.header.cmd = IO_STATE;
        pub_msg.header.target_mode = ID;
        pub_msg.header.target = msg->header.source;
        pub_msg.header.size = sizeof(char);

        if (module == pin[P5])
        {
            pub_msg.data[0] = (char)(HAL_GPIO_ReadPin(P5_GPIO_Port, P5_Pin) > 0);
        }
        else if (module == pin[P6])
        {
            pub_msg.data[0] = (char)(HAL_GPIO_ReadPin(P6_GPIO_Port, P6_Pin) > 0);
        }
        else
        {
            return;
        }
        Luos_SendMsg(module, &pub_msg);
        return;
    }
}

static void rx_digit_write_cb(module_t *module, msg_t *msg)
{
    if (msg->header.cmd == IO_STATE)
    {
        // we have to update pin state
        if (module == pin[P2])
        {
            HAL_GPIO_WritePin(P2_GPIO_Port, P2_Pin, msg->data[0]);
        }
        if (module == pin[P3])
        {
            HAL_GPIO_WritePin(P3_GPIO_Port, P3_Pin, msg->data[0]);
        }
        if (module == pin[P4])
        {
            HAL_GPIO_WritePin(P4_GPIO_Port, P4_Pin, msg->data[0]);
        }
    }
}

static void rx_analog_read_cb(module_t *module, msg_t *msg)
{
    if (msg->header.cmd == ASK_PUB_CMD)
    {
        msg_t pub_msg;
        voltage_t volt;
        if (module == pin[P1])
        {
            volt = ((float)analog_input.p1 / 4096.0f) * 3.3f;
        }
        else if (module == pin[P7])
        {
            volt = ((float)analog_input.p7 / 4096.0f) * 3.3f;
        }
        else if (module == pin[P8])
        {
            volt = ((float)analog_input.p8 / 4096.0f) * 3.3f;
        }
        else if (module == pin[P9])
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
        voltage_to_msg(&volt, &pub_msg);
        Luos_SendMsg(module, &pub_msg);
        return;
    }
}
