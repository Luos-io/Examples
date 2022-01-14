/******************************************************************************
 * @file pipe_buffer
 * @brief communication buffer
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include <stdbool.h>
#include <string.h>
#include "luos_utils.h"

#include "pipe_com.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define PIPE_TO_LUOS_MAX_TASK 50

/*******************************************************************************
 * Variables
 ******************************************************************************/
uint8_t P2L_Buffer[PIPE_TO_LUOS_BUFFER_SIZE] = {0};
uint16_t P2LTask[PIPE_TO_LUOS_MAX_TASK]      = {0};
uint8_t P2LTaskID                            = 0;
volatile uint16_t P2LBuffer_NbrdataIn        = 0;

uint8_t L2P_Buffer[LUOS_TO_PIPE_BUFFER_SIZE] = {0};
volatile uint8_t L2P_Buffer_WritePointer     = 0;

/*******************************************************************************
 * Function
 ******************************************************************************/
static void PipeBuffer_ClearP2LTask(void);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void PipeBuffer_Init(void)
{
    memset((void *)P2LTask, 0, sizeof(P2LTask));
    P2LBuffer_NbrdataIn = 0;
    P2LTaskID           = 0;
}
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
uint8_t *PipeBuffer_GetP2LBuffer(void)
{
    return &P2L_Buffer[0];
}
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
uint8_t PipeBuffer_GetP2LTask(uint16_t *task)
{
    if (P2LTaskID != 0)
    {
        *task = P2LTask[0];
        P2LBuffer_NbrdataIn -= P2LTask[0];
        PipeBuffer_ClearP2LTask();
        return true;
    }
    return false;
}
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void PipeBuffer_ClearP2LTask(void)
{
    uint8_t i = 0;
    for (i = 0; i < P2LTaskID; i++)
    {
        P2LTask[i] = P2LTask[i + 1];
    }
    if (P2LTaskID != 0)
    {
        P2LTaskID--;
        P2LTask[P2LTaskID] = 0;
    }
}
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void PipeBuffer_AllocP2LTask(uint16_t size)
{
    P2LBuffer_NbrdataIn += size;
    if (P2LBuffer_NbrdataIn > PIPE_TO_LUOS_BUFFER_SIZE)
    {
        LUOS_ASSERT(0);
    }
    else
    {
        for (uint8_t i = 0; i < PIPE_TO_LUOS_MAX_TASK; i++)
        {
            if (P2LTask[i] == 0)
            {
                P2LTask[i] = size;
                P2LTaskID++;
                Stream_AddAvailableSampleNB(get_P2L_StreamChannel(), size);
                return;
            }
        }
    }
    // No more space
    LUOS_ASSERT(0);
}
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
uint8_t *PipeBuffer_GetL2PBuffer(void)
{
    return &L2P_Buffer[0];
}
