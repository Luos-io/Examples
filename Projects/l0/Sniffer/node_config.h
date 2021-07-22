#ifndef _NODE_CONFIG_H_
#define _NODE_CONFIG_H_

/*******************************************************************************
 * PROJECT DEFINITION
*******************************************************************************/

/*******************************************************************************
 * LUOS LIBRARY DEFINITION
*******************************************************************************
 *    Define                | Default Value              | Description
 *    :---------------------|------------------------------------------------------
 *    MAX_CONTAINER_NUMBER  |              5             | Service number in the node
 *    MSG_BUFFER_SIZE       | 3*SIZE_MSG_MAX (405 Bytes) | Size in byte of the Luos buffer TX and RX
 *    MAX_MSG_NB            |   2*MAX_CONTAINER_NUMBER   | Message number in Luos buffer
 *    NBR_PORT              |              2             | PTP Branch number Max 8
 *    NBR_RETRY             |              10            | Send Retry number in case of NACK or collision
******************************************************************************/
#define MAX_CONTAINER_NUMBER 1
#define MSG_BUFFER_SIZE      2048
#define MAX_MSG_NB           60

/*******************************************************************************
 * LUOS HAL LIBRARY DEFINITION
*******************************************************************************
 *    Define                  | Description
 *    :-----------------------|-----------------------------------------------
 *    MCUFREQ                 | Put your the MCU frequency (value in Hz)
 *    TIMERDIV                | Timer divider clock (see your clock configuration)
 *    USE_CRC_HW              | define to 0 if there is no Module CRC in your MCU
 *    USE_TX_IT               | define to 1 to not use DMA transfers for Luos Tx
 *
 *    PORT_CLOCK_ENABLE       | Enable clock for port
 *    PTPx                    | A,B,C,D etc. PTP Branch Pin/Port/IRQ
 *    TX_LOCK_DETECT          | Disable by default use if not busy flag in USART Pin/Port/IRQ
 *    RX_EN                   | Rx enable for driver RS485 always on Pin/Port
 *    TX_EN                   | Tx enable for driver RS485 Pin/Port
 *    COM_TX                  | Tx USART Com Pin/Port/Alternate
 *    COM_RX                  | Rx USART Com Pin/Port/Alternate
 *    PINOUT_IRQHANDLER       | Callback function for Pin IRQ handler
 *    LUOS_COM_CLOCK_ENABLE   | Enable clock for USART
 *    LUOS_COM                | USART number
 *    LUOS_COM_IRQ            | USART IRQ number
 *    LUOS_COM_IRQHANDLER     | Callback function for USART IRQ handler
 *    LUOS_DMA_CLOCK_ENABLE   | Enable clock for DMA
 *    LUOS_DMA                | DMA number
 *    LUOS_DMA_CHANNEL        | DMA channel (depending on MCU DMA may need special config)
 *    LUOS_TIMER_CLOCK_ENABLE | Enable clock for Timer
 *    LUOS_TIMER              | Timer number
 *    LUOS_TIMER_IRQ          | Timer IRQ number
 *    LUOS_TIMER_IRQHANDLER   | Callback function for Timer IRQ handler
 *    FLASH_SECTOR               | FLASH page size
 *    PAGE_SIZE               | FLASH page size
 *    ADDRESS_LAST_PAGE_FLASH | Page to write alias
******************************************************************************/

/*  Sniffer should not have enabled PTP ports, as it should not be poked or detected 
 *  by the other nodes, so we disable the PTPA and PTPB ports   */
#define PPTA_PORT 0
#define PPTA_PIN  0
#define PPTA_IRQ  0
#define PPTB_PORT 0
#define PPTB_PIN  0
#define PPTB_IRQ  0
#define NBR_PORT  0

#endif /* _NODE_CONFIG_H_ */