/******************************************************************************
 * @file dxl
 * @brief driver example a simple dxl
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#ifndef DXL_H
#define DXL_H

#include "luos.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define MAX_ID            40
#define DXL_TIMEOUT       2
#define TEMP_REFRESH_MS   1000
#define FACTORY_RESET_REG 0xFF
/*******************************************************************************
 * Variables
 ******************************************************************************/
typedef enum
{
    AX12  = (12 + (0 << 8)),
    AX18  = (18 + (0 << 8)),
    RX24  = (24 + (0 << 8)),
    RX28  = (28 + (0 << 8)),
    MX28  = (29 + (0 << 8)),
    RX64  = (64 + (0 << 8)),
    MX12  = (104 + (1 << 8)),
    MX64  = (54 + (1 << 8)),
    MX106 = (64 + (1 << 8)),
    XL320 = (94 + (1 << 8))
} dxl_models_t;

typedef enum
{
    MODE_WHEEL,
    MODE_ANGLE,
    MODE_ANGLE_LIMIT,
    MODE_POWER_LIMIT,
    MODE_PID,
    MODE_SPEED,
    MODE_COMPLIANT,
    MODE_DETECT,
    MODE_TEMP,
    MODE_REG,
    MODE_ID
} dxl_mode_t;

typedef struct
{
    dxl_mode_t mode;
    int reg;
    float val;
    float val2;
    container_t *container_pointer;
} dxl_t;
/*******************************************************************************
 * Function
 ******************************************************************************/
void Dxl_Init(void);
void Dxl_Loop(void);

#endif /* DXL_H */
