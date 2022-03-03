/******************************************************************************
 * @file motor
 * @brief FOC motor driver
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef MOTOR_H
#define MOTOR_H

#include "luos.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void Motor_Init(void);
void Motor_Loop(void);

#endif /* MOTOR_H */
