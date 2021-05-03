#include "usart.h"
#include "json_alloc.h"
#include <string.h>
#include "luos_utils.h"

volatile uint8_t is_sending = 0;

void json_pipe_init(void)
{
    LL_USART_ClearFlag_IDLE(USART3);
    LL_USART_EnableIT_IDLE(USART3);
    //NVIC_DisableIRQ(DMA1_Channel2_3_IRQn);
    LL_DMA_DisableIT_TC(DMA1, LL_DMA_CHANNEL_6);
    LL_DMA_DisableIT_HT(DMA1, LL_DMA_CHANNEL_6);
    LL_DMA_DisableIT_TE(DMA1, LL_DMA_CHANNEL_6);
    LL_DMA_SetM2MDstAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)json_alloc_get_rx_buf());
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_6, JSON_BUFF_SIZE);
    LL_DMA_SetM2MSrcAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)&USART3->RDR);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_6);
    LL_USART_EnableDMAReq_RX(USART3);
    //HAL_UART_Receive_DMA(&huart3, (void *)json_alloc_get_rx_buf(), JSON_BUFF_SIZE);

    LL_DMA_DisableIT_HT(DMA1, LL_DMA_CHANNEL_7);
    LL_DMA_DisableIT_TE(DMA1, LL_DMA_CHANNEL_7);
}

void json_pipe_send(void)
{
    if ((!is_sending))
    {
        char *ready_json = json_alloc_get_tx_task();
        if (ready_json != 0)
        {
            uint32_t size = strlen(ready_json);
            if (size > JSON_BUFF_SIZE)
            {
                size = JSON_BUFF_SIZE;
            }
            HAL_UART_Transmit_DMA(&huart3, (void *)ready_json, size);
            is_sending = 1;
        }
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        // remove the sent tx message from buffer
        json_alloc_pull_tx_task();
        char *ready_json = json_alloc_get_tx_task();
        if (ready_json == 0)
        {
            is_sending = 0;
        }
        else
        {

            uint32_t size = strlen(ready_json);
            if (size > JSON_BUFF_SIZE)
            {
                size = JSON_BUFF_SIZE;
            }
            HAL_UART_Transmit_DMA(&huart3, (void *)ready_json, size);
        }
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3)
    {
        // we should not come here there is no more space on rx buffer. Assert
        LUOS_ASSERT(0);
    }
}

void USART3_4_IRQHandler(void)
{
    // check if we receive an IDLE on usart3
    if (LL_USART_IsActiveFlag_IDLE(USART3))
    {
        LL_USART_ClearFlag_IDLE(USART3);
        // reference received Json and get the next one
        char *addr = json_alloc_set_rx_task(JSON_BUFF_SIZE - LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_6) - 1);

        // HAL_UART_DMAStop(&huart3);
        // HAL_UART_Receive_DMA(&huart3, (void *)addr, JSON_BUFF_SIZE);
        __disable_irq();
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_6);
        LL_DMA_SetM2MDstAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)addr);
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_6, JSON_BUFF_SIZE);
        LL_DMA_SetM2MSrcAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)&USART3->RDR);
        LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_6, (uint32_t)addr);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_6);
        LL_USART_EnableDMAReq_RX(USART3);
        __enable_irq();
    }
    HAL_UART_IRQHandler(&huart3);
}
