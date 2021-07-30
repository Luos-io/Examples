/******************************************************************************
 * @file data_manager
 * @brief Manage data conversion strategy.
 * @author Luos
 ******************************************************************************/
#ifndef DATA_MNGR_H
#define DATA_MNGR_H

#include "luos.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
extern time_luos_t update_time;
extern char detection_ask;

/*******************************************************************************
 * Function
 ******************************************************************************/

// This function will manage msg collection from sensors
void DataManager_collect(service_t *service);

// This function manage entirely data conversion
void DataManager_Run(service_t *service);

// This function manage only commande incoming from pipe
void DataManager_RunPipeOnly(service_t *service);

#endif /* DATA_MNGR_H */