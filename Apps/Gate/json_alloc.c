#include "cmd.h"
#include <stdio.h>
#include "json_alloc.h"
#include "json_pipe.h"

// Json RX buffer
volatile char rx_buf[JSON_RX_BUF_NUM][JSON_BUFF_SIZE] = {0};
volatile uint8_t current_rx                           = 0;
volatile char *rx_json_task[JSON_RX_BUF_NUM];

// Json TX buffer
volatile char tx_buf[JSON_TX_BUF_NUM][JSON_BUFF_SIZE] = {0};
volatile uint8_t current_tx                           = 0;
volatile char *tx_json_task[JSON_TX_BUF_NUM];

void json_alloc_reinit(void)
{
    // Reinit RX
    for (int i = 0; i < JSON_RX_BUF_NUM; i++)
    {
        rx_json_task[i] = 0;
    }
    // Do not reset current_rx to 0 because DMA is already listening into the current_rx buffer.
    // Reinit TX
    for (int i = 0; i < JSON_TX_BUF_NUM; i++)
    {
        tx_json_task[i] = 0;
    }
    current_tx = 0;
}

//*************** RX buffer management **************

char *json_alloc_get_rx_buf(void)
{
    return (char *)rx_buf[current_rx];
}
char *json_alloc_get_rx_task(void)
{
    return (char *)rx_json_task[0];
}

char *json_alloc_pull_rx_task(void)
{
    char *ret_json = (char *)rx_json_task[0];
    for (int i = 0; i < JSON_RX_BUF_NUM; i++)
    {
        if (rx_json_task[i] == 0)
        {
            return ret_json;
        }
        rx_json_task[i] = rx_json_task[i + 1];
    }
    rx_json_task[JSON_RX_BUF_NUM - 1] = 0;
    return ret_json;
}

char *json_alloc_set_rx_task(uint16_t carac_nbr)
{
    LUOS_ASSERT(
        (current_rx < JSON_RX_BUF_NUM) || (current_rx >= 0) || (carac_nbr < JSON_BUFF_SIZE));

    if (rx_buf[current_rx][carac_nbr] == '\r')
    {
        rx_buf[current_rx][carac_nbr] = '\0';
        // We have a complete Json here
        // Create a task for it
        for (int i = 0; i < JSON_RX_BUF_NUM; i++)
        {
            if (rx_json_task[i] == 0)
            {
                rx_json_task[i] = rx_buf[current_rx];
                current_rx++;
                if (current_rx >= JSON_RX_BUF_NUM)
                {
                    current_rx = 0;
                }
                return (char *)rx_buf[current_rx];
            }
        }
        // No more space on rx_json_task
        LUOS_ASSERT(0);
    }
    return (char *)rx_buf[current_rx];
}

//*************** TX buffer management **************
char *json_alloc_get_tx_buf(void)
{
    return (char *)tx_buf[current_tx];
}
char *json_alloc_get_tx_task(void)
{
    return (char *)tx_json_task[0];
}

char *json_alloc_pull_tx_task(void)
{
    int i;
    char *ret_json = (char *)tx_json_task[0];
    for (i = 0; i < JSON_TX_BUF_NUM; i++)
    {
        if (tx_json_task[i] == 0)
        {
            return ret_json;
        }
        tx_json_task[i] = tx_json_task[i + 1];
    }
    tx_json_task[i] = 0;
    return ret_json;
}

// set a Tx task on the current TX buffer and send back the next buffer available
char *json_alloc_set_tx_task(uint16_t carac_nbr)
{
    LUOS_ASSERT((current_tx < JSON_TX_BUF_NUM) || (current_tx >= 0) || (carac_nbr < JSON_BUFF_SIZE));
    for (int i = 0; i < JSON_TX_BUF_NUM; i++)
    {
        if (tx_json_task[i] == 0)
        {
            tx_json_task[i] = tx_buf[current_tx];
            current_tx++;
            if (current_tx >= JSON_TX_BUF_NUM)
            {
                current_tx = 0;
            }
            json_pipe_send();
            return (char *)tx_buf[current_tx];
        }
    }
    // No more space on tx_json_task
    LUOS_ASSERT(0);
    return 0;
}
