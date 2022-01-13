/******************************************************************************
 * @file servo_drv
 * @brief driver example a simple servo motor
 * @author mariebidouille
 * @version 0.0.0
 ******************************************************************************/
#ifndef SERVO_DRV_H
#define SERVO_DRV_H

#include "luos.h"
#include "luos_hal.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
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

typedef struct
{
    TIM_TypeDef *Timer;
    uint32_t Channel;
} servo_control_t;

typedef struct
{
    angular_position_t angle;
    servo_parameters_t param;
    servo_control_t control;
} servo_t;

//HW definition

#define DEFAULT_PRESACALER 60

#define PWM_PIN_CLK()                 \
    do                                \
    {                                 \
        __HAL_RCC_GPIOA_CLK_ENABLE(); \
        __HAL_RCC_GPIOB_CLK_ENABLE(); \
    } while (0U)

#define PWM_TIMER_CLK()              \
    do                               \
    {                                \
        __HAL_RCC_TIM2_CLK_ENABLE(); \
    } while (0U)

#define SERVO_PIN     GPIO_PIN_0
#define SERVO_PORT    GPIOA
#define SERVO_AF      GPIO_AF1_TIM2
#define SERVO_TIMER   TIM2
#define SERVO_CHANNEL LL_TIM_CHANNEL_CH1

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Functions
 ******************************************************************************/
void ServoDrv_Init(void);

uint8_t ServoDrv_SetPosition(angular_position_t angle);
uint8_t ServoDrv_Parameter(servo_parameters_t param);

#endif /* SERVO_DRV_H */
