/******************************************************************************
 * @file product_config
 * @brief The official Luos demo
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef PRODUCT_CONFIG_H
#define PRODUCT_CONFIG_H
#include "luos.h"

enum
{
    LEDSTRIP_POSITION_APP = LUOS_LAST_TYPE,
    RUN_MOTOR
};

enum
{
    NO_MOTOR,
    MOTOR_1_POSITION,
    MOTOR_2_POSITION,
    MOTOR_3_POSITION,
};

#endif //PRODUCT_CONFIG_H