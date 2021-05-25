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
void DataManager_collect(container_t *service);

// This function manage entirely data conversion
void DataManager_Run(container_t *service);

#endif /* DATA_MNGR_H */