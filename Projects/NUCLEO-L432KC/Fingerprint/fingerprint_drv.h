/******************************************************************************
 * @file fingerprint_drv
 * @brief Driver example for fingerprint sensor 
 * @author mariebidouille
 * @version 0.0.0
 ******************************************************************************/
#ifndef FINGERPRINT_DRV_H
#define FINGERPRINT_DRV_H

#include "fingerprint_com.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Functions
 ******************************************************************************/
void FingerprintDrv_Init(void);

uint8_t FingerprintDrv_Enroll(void);
uint8_t FingerprintDrv_CheckAuth(void);
uint8_t FingerprintDrv_DeleteAll(void);

#endif /* FINGERPRINT_DRV_H */