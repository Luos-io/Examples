/******************************************************************************
 * @file gate
 * @brief Container gate
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "main.h"
#include "gate.h"
#include "json_mnger.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s
/*******************************************************************************
 * Variables
 ******************************************************************************/
container_t *container;
msg_t msg;
uint8_t RxData;
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
    LL_USART_ClearFlag_IDLE(USART3);
    LL_USART_EnableIT_IDLE(USART3);
    NVIC_DisableIRQ(DMA1_Channel2_3_IRQn);
    LL_DMA_DisableIT_TC(DMA1, LL_DMA_CHANNEL_3);
    LL_DMA_DisableIT_HT(DMA1, LL_DMA_CHANNEL_3);
    LL_DMA_DisableIT_TE(DMA1, LL_DMA_CHANNEL_3);
    LL_DMA_SetM2MDstAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t)get_json_buf());
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, JSON_BUFF_SIZE);
    LL_DMA_SetM2MSrcAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t)&USART3->RDR);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);
    LL_USART_EnableDMAReq_RX(USART3);
    container = Luos_CreateContainer(0, GATE_MOD, "gate", STRINGIFY(VERSION));
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

    // Check if there is a dead container
    if (container->ll_container->dead_container_spotted)
    {
        char json[JSON_BUFF_SIZE] = {0};
        exclude_container_to_json(container->ll_container->dead_container_spotted, json);
#ifdef USE_SERIAL
        serial_write(json, strlen(json));
#else
        printf(json);
#endif
        container->ll_container->dead_container_spotted = 0;
    }
    if (detection_done)
    {
        char json[JSON_BUFF_SIZE] = {0};
        state = !state;
        format_data(container, json);
        if (json[0] != '\0')
        {
#ifdef USE_SERIAL
            serial_write(json, strlen(json));
#else
            printf(json);
#endif
            keepAlive = 0;
        }
        else
        {
            if (keepAlive > 200)
            {
#ifdef USE_SERIAL
                sprintf(json, "{}\n");
                serial_write(json, strlen(json));
#else
                printf("{}\n");
#endif
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
#ifdef USE_SERIAL
        serial_write(json, strlen(json));
#else
        printf(json);
#endif
        detection_done = 1;
        detection_ask = 0;
    }
    HAL_Delay(get_delay());
}

void USART3_4_IRQHandler(void)
{
    // check if we receive an IDLE on usart3
    if (LL_USART_IsActiveFlag_IDLE(USART3))
    {
        LL_USART_ClearFlag_IDLE(USART3);
        // check DMA data
        check_json(JSON_BUFF_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_3) - 1);

        // reset DMA
        __disable_irq();
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_3);
        char *addr = get_json_buf();
        LL_DMA_SetM2MDstAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t)addr);
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_3, JSON_BUFF_SIZE);
        LL_DMA_SetM2MSrcAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t)&USART3->RDR);
        LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_3, (uint32_t)addr);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_3);
        LL_USART_EnableDMAReq_RX(USART3);
        __enable_irq();
    }
}

#ifdef USE_SERIAL
int serial_write(char *data, int len)
{
    for (unsigned short i = 0; i < len; i++)
    {
        while (!LL_USART_IsActiveFlag_TXE(USART3))
            ;
        LL_USART_TransmitData8(USART3, *(data + i));
    }
    return 0;
}
#endif
