/******************************************************************************
 * @file pipe_com
 * @brief communication driver
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/

#include <Arduino.h>

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdbool.h>
#include "pipe_com.h"
#include "main.h"
#include "luos_utils.h"

#ifdef __cplusplus
}
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile uint8_t is_sending = false;
volatile uint16_t size_to_send = 0;
/*******************************************************************************
 * Function
 ******************************************************************************/

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void PipeCom_Init(void)
{
    Serial.begin(1000000);
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
    }
    PipeBuffer_Init();
}
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void PipeCom_SetP2L(uint8_t *data, uint16_t size)
{
    Stream_PutSample(get_P2L_StreamChannel(), data, size);
}
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
volatile uint8_t PipeCom_SendL2PPending(void)
{
    return is_sending;
}
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void PipeCom_SendL2P(uint8_t *data, uint16_t size)
{
    is_sending = true;
    size_to_send = size;
    while(size_to_send != 0)
    {
        Serial.write(data++);
        size_to_send--;
    }
}
