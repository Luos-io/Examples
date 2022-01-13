/******************************************************************************
 * @file biometric_security
 * @brief app example 
 * @author mariebidouille
 * @version 0.0.0
 ******************************************************************************/
#ifndef BIOMETRIC_SECURITY_H
#define BIOMETRIC_SECURITY_H

#include "luos.h"
#include "luos_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define UPDATE_PERIOD_MS 200

typedef enum 
{
    BIOMETRIC_SECURITY_TYPE = LUOS_LAST_TYPE
}security_system_type_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Functions
 ******************************************************************************/
void BiometricSecurity_Init(void);
void BiometricSecurity_Loop(void);

#endif /* BIOMETRIC_SECURITY_H */