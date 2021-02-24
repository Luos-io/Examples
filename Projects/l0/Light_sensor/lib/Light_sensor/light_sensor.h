/******************************************************************************
 * @file light sensor
 * @brief driver example a simple light sensor
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

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
void LightSensor_Init(void);
void LightSensor_Loop(void);

#endif /* LIGHT_SENSOR_H */