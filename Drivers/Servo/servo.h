/******************************************************************************
 * @file servo
 * @brief driver example a simple servo motor
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef SERVO_H
#define SERVO_H

#include "luos.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*
 * Servo
 */
typedef struct
{
    union
    {
        struct __attribute__((__packed__))
        {
            angular_position_t max_angle;
            float min_pulse_time;
            float max_pulse_time;
        };
        unsigned char unmap[3 * sizeof(float)];
    };
} servo_parameters_t;
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
void Servo_Init(void);
void Servo_Loop(void);

#endif /* SERVO_H */
