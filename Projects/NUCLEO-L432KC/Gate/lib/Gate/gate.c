/******************************************************************************
 * @file gate
 * @brief Container gate
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "gate.h"
#include "json_mnger.h"
#include <stdio.h>

#ifdef USE_SERIAL
#include "stm32l4xx_ll_usart.h"
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifdef USE_SERIAL
static int serial_write(char *data, int len);
static void Gpio_Init(void);
static void Usart_Init(void);
#endif

#ifndef REV
#define REV {1,0,0}
#endif
/*******************************************************************************
 * Variables
 ******************************************************************************/
container_t *container;
msg_t msg;
char* RxData;
uint16_t RxDataCtn;
container_t *container_pointer;
volatile msg_t pub_msg;

volatile int pub = LUOS_PROTOCOL_NB;
/*******************************************************************************
 * Function
 ******************************************************************************/
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Gate_Init(void)
{
    revision_t revision = {.unmap = REV};
#ifdef USE_SERIAL
    Gpio_Init();
    Usart_Init();
    RxData = get_json_buf();
    RxDataCtn = 0;
#endif
    container = Luos_CreateContainer(0, GATE_MOD, "gate", revision);
}

__attribute__((weak)) void json_send(char *json)
{
#ifdef USE_SERIAL
    serial_write(json, strlen(json));
#else
    printf(json);
#endif
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Gate_Loop(void)
{
    static unsigned int keepAlive = 0;
    static volatile uint8_t detection_done = 0;
    static char state = 0;
    uint32_t tickstart = 0;

    // Check if there is a dead container
    if (container->ll_container->dead_container_spotted)
    {
        char json[JSON_BUFF_SIZE] = {0};
        exclude_container_to_json(container->ll_container->dead_container_spotted, json);
        json_send(json);
        container->ll_container->dead_container_spotted = 0;
    }
    if (detection_done)
    {
        char json[JSON_BUFF_SIZE] = {0};
        state = !state;
        format_data(container, json);
        if (json[0] != '\0')
        {
            json_send(json);
            keepAlive = 0;
        }
        else
        {
            if (keepAlive > 200)
            {
                sprintf(json, "{}\n");
                json_send(json);
            }
            else
            {
                keepAlive++;
            }
        }
        collect_data(container);
    }
    if (pub != LUOS_PROTOCOL_NB)
    {
        Luos_SendMsg(container_pointer, (msg_t *)&pub_msg);
        pub = LUOS_PROTOCOL_NB;
    }
    // check if serial input messages ready and convert it into a luos message
    send_cmds(container);
    if (detection_ask)
    {
        char json[JSON_BUFF_SIZE * 2] = {0};
        RoutingTB_DetectContainers(container);
        routing_table_to_json(json);
        json_send(json);
        detection_done = 1;
        detection_ask = 0;
    }

    tickstart = Luos_GetSystick();
    while((Luos_GetSystick() - tickstart) < 1);

}

#ifdef USE_SERIAL
/******************************************************************************
 * @brief
 * @param None
 * @return None
 ******************************************************************************/
static void Gpio_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	__HAL_RCC_GPIOA_CLK_ENABLE();

	GPIO_InitStruct.Pin = GPIO_PIN_2;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	GPIO_InitStruct.Alternate = GPIO_AF3_USART2;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
/******************************************************************************
 * @brief
 * @param None
 * @return None
 ******************************************************************************/
static void Usart_Init(void)
{
  __HAL_RCC_USART2_CLK_ENABLE();

	LL_USART_InitTypeDef USART_InitStruct;

	LL_USART_Disable(USART2);
	USART_InitStruct.BaudRate = 1000000;
	USART_InitStruct.DataWidth = LL_USART_DATAWIDTH_8B;
	USART_InitStruct.StopBits = LL_USART_STOPBITS_1;
	USART_InitStruct.Parity = LL_USART_PARITY_NONE;
	USART_InitStruct.TransferDirection = LL_USART_DIRECTION_TX_RX;
	USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
	USART_InitStruct.OverSampling = LL_USART_OVERSAMPLING_16;
	while (LL_USART_Init(USART2, &USART_InitStruct) != SUCCESS)
		;
	LL_USART_Enable(USART2);

	// Enable Reception interrupt
	LL_USART_EnableIT_RXNE(USART2);

	HAL_NVIC_EnableIRQ(USART2_IRQn);
	HAL_NVIC_SetPriority(USART2_IRQn, 0, 1);
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void USART2_IRQHandler(void)
{
	if ((LL_USART_IsActiveFlag_RXNE(USART2) != RESET) && (LL_USART_IsEnabledIT_RXNE(USART2) != RESET))
	{
		*RxData = LL_USART_ReceiveData8(USART2);
		RxDataCtn++;
		if(*RxData == '\r')
		{
		  check_json(RxDataCtn-1);
		  RxData = get_json_buf();
		  RxDataCtn = 0;
		}
		else
		{
		  RxData++;
		}
	}
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
static int serial_write(char *data, int len)
{
    for (unsigned short i = 0; i < len; i++)
    {
        while (!LL_USART_IsActiveFlag_TXE(USART2))
            ;
        LL_USART_TransmitData8(USART2, *(data + i));
    }
    return 0;
}
#endif
