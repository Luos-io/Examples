/******************************************************************************
 * @file analog
 * @brief analog for this project
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef __ANALOG_H
#define __ANALOG_H

#include "main.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
ADC_HandleTypeDef ControlledMotor_adc;
DMA_HandleTypeDef ControlledMotor_dma_adc;

// This structure need to list all ADC configured in the good order determined by the
// ADC_CHANEL number in increasing order
typedef struct __attribute__((__packed__))
{
    union
    {
        struct __attribute__((__packed__))
        {
            uint32_t current;
        };
        uint32_t unmap[3]; /*!< Unmaped form. */
    };
} analog_input_t;

volatile analog_input_t analog_input;

#endif /*__ __ANALOG_H */
