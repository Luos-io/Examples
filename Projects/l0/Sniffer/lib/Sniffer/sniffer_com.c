/******************************************************************************
 * @file sniffer_com
 * @brief communication driver
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "sniffer_com.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile uint8_t is_sending    = 0;
volatile uint16_t size_to_send = 0;
/*******************************************************************************
 * Functions
 ******************************************************************************/
void SnifferCom_DMAInit(void);

/******************************************************************************
 * @brief Initialization of the sniffer driver
 * @param None
 * @return None
 ******************************************************************************/
void SnifferCom_Init(void)
{
    SNIFFER_TX_CLK();
    SNIFFER_RX_CLK();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    //TX
    GPIO_InitStruct.Pin       = SNIFFER_TX_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = SNIFFER_TX_AF;
    HAL_GPIO_Init(SNIFFER_TX_PORT, &GPIO_InitStruct);
    //RX
    GPIO_InitStruct.Pin       = SNIFFER_RX_PIN;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_PULLUP;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = SNIFFER_RX_AF;
    HAL_GPIO_Init(SNIFFER_RX_PORT, &GPIO_InitStruct);
    // Initialise USART3
    SNIFFER_COM_CLOCK_ENABLE();
    LL_USART_InitTypeDef USART_InitStruct = {0};
    LL_USART_Disable(SNIFFER_COM);
    USART_InitStruct.BaudRate            = 1000000;
    USART_InitStruct.DataWidth           = LL_USART_DATAWIDTH_8B;
    USART_InitStruct.StopBits            = LL_USART_STOPBITS_1;
    USART_InitStruct.Parity              = LL_USART_PARITY_NONE;
    USART_InitStruct.TransferDirection   = LL_USART_DIRECTION_TX_RX;
    USART_InitStruct.HardwareFlowControl = LL_USART_HWCONTROL_NONE;
    USART_InitStruct.OverSampling        = LL_USART_OVERSAMPLING_16;
    while (LL_USART_Init(SNIFFER_COM, &USART_InitStruct) != SUCCESS)
        ;
    LL_USART_Enable(SNIFFER_COM);

    LL_USART_ClearFlag_IDLE(SNIFFER_COM);
    LL_USART_EnableIT_IDLE(SNIFFER_COM);

    HAL_NVIC_EnableIRQ(SNIFFER_COM_IRQ);
    HAL_NVIC_SetPriority(SNIFFER_COM_IRQ, 0, 1);

    SnifferCom_DMAInit();
}

/******************************************************************************
 * @brief Initialization of Sniffer DMA driver
 * @param None
 * @return None
 ******************************************************************************/
void SnifferCom_DMAInit(void)
{
    SNIFFER_SEND_DMA_CLOCK_ENABLE();
    SNIFFER_RCV_DMA_CLOCK_ENABLE();

    //Sniffer Receive DMA
    LL_DMA_DisableChannel(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL);
    LL_DMA_SetDataTransferDirection(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
    LL_DMA_SetChannelPriorityLevel(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL, LL_DMA_MODE_CIRCULAR);
    LL_DMA_SetPeriphIncMode(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL, LL_DMA_MDATAALIGN_BYTE);
    LL_SYSCFG_SetRemapDMA_USART(SNIFFER_USART_RMP_DMA);

    //Prepare buffer
    LL_DMA_SetPeriphAddress(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL, (uint32_t)&SNIFFER_COM->RDR);
    LL_DMA_SetMemoryAddress(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL, (uint32_t)get_cmd_buf());
    LL_USART_EnableDMAReq_RX(SNIFFER_COM);
    LL_DMA_EnableChannel(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL);

    //Sniffer Send DMA
    LL_DMA_SetDataTransferDirection(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
    LL_DMA_SetChannelPriorityLevel(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL, LL_DMA_PRIORITY_LOW);
    LL_DMA_SetMode(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL, LL_DMA_MODE_NORMAL);
    LL_DMA_SetPeriphIncMode(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL, LL_DMA_PERIPH_NOINCREMENT);
    LL_DMA_SetMemoryIncMode(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL, LL_DMA_MEMORY_INCREMENT);
    LL_DMA_SetPeriphSize(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL, LL_DMA_PDATAALIGN_BYTE);
    LL_DMA_SetMemorySize(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL, LL_DMA_MDATAALIGN_BYTE);
    LL_SYSCFG_SetRemapDMA_USART(SNIFFER_USART_RMP_DMA);

    //Prepare buffer
    LL_DMA_SetPeriphAddress(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL, (uint32_t)&SNIFFER_COM->TDR);
    LL_USART_EnableDMAReq_TX(SNIFFER_COM);
    HAL_NVIC_EnableIRQ(SNIFFER_SEND_DMA_IRQ);
    HAL_NVIC_SetPriority(SNIFFER_SEND_DMA_IRQ, 0, 1);

    LL_DMA_EnableIT_TC(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL);
}

/*******************************************************************************
 * @brief Function that passes the data via DMA
 * @param the address of data to send, the size of data to send
 * @return None
 ******************************************************************************/
void SnifferCom_Send(uint8_t *data, uint16_t size)
{
    is_sending   = 1;
    size_to_send = size;
    LL_DMA_DisableChannel(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL);
    LL_DMA_SetMemoryAddress(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL, (uint32_t)data);
    LL_DMA_SetDataLength(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL, size);
    LL_DMA_EnableChannel(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL);
}

/******************************************************************************
 * @brief gives the situation of data transmission
 * @param None
 * @return 0 if DMA is empty, 1 if there are still data being transmitted
 ******************************************************************************/
uint8_t SnifferCom_Pending(void)
{
    return is_sending;
}

/******************************************************************************
 * @brief sniffer serial com  interrupt handler
 * @param None
 * @return None
 ******************************************************************************/
void SNIFFER_COM_IRQHANDLER()
{
    // check if we receive an IDLE on usart3
    if (LL_USART_IsActiveFlag_IDLE(SNIFFER_COM) != RESET)
    {
        LL_USART_ClearFlag_IDLE(SNIFFER_COM);
        // reinit DMA receive channel
        LL_DMA_DisableChannel(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL);
        LL_DMA_SetMemoryAddress(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL, (uint32_t)get_cmd_buf());
        LL_DMA_SetDataLength(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL, CMD_BUFF_SIZE);
        LL_DMA_EnableChannel(SNIFFER_RCV_DMA, SNIFFER_RCV_DMA_CHANNEL);
    }

    SnifferCom_DMAInit();
}

/******************************************************************************
 * @brief sniffer transmission  dma interrupt handler
 * @param None
 * @return None
 ******************************************************************************/
void SNIFFER_SEND_DMA_IRQHANDLER()
{

    uint16_t size = 0;
    // check if we receive an IDLE on usart3
    if ((SNIFFER_SEND_DMA_TC(SNIFFER_SEND_DMA) != RESET) && (LL_DMA_IsEnabledIT_TC(SNIFFER_SEND_DMA, SNIFFER_SEND_DMA_CHANNEL) != RESET))
    {
        SNIFFER_SEND_DMA_CLEAR_TC(SNIFFER_SEND_DMA);
        Stream_RmvAvailableSampleNB(get_Sniffer_StreamChannel(), size_to_send);
        size = Stream_GetAvailableSampleNBUntilEndBuffer(get_Sniffer_StreamChannel());
        if (size > 0)
        {
            SnifferCom_Send(get_Sniffer_StreamChannel()->sample_ptr, size);
        }
        else
        {
            is_sending = 0;
        }
    }
}
