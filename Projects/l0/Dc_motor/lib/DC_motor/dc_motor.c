/******************************************************************************
 * @file dc_motor
 * @brief driver example a simple dc_motor
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "main.h"
#include "dc_motor.h"
#include "tim.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define MOTORNUMBER 2
/*******************************************************************************
 * Variables
 ******************************************************************************/
container_t *container[MOTORNUMBER];
/*******************************************************************************
 * Function
 ******************************************************************************/
static void MotorDC_MsgHandler(container_t *container, msg_t *msg);
static int find_id(container_t *my_container);
static void set_power(container_t *container, ratio_t power);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void MotorDC_Init(void)
{
    revision_t revision = {.unmap = REV};
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    container[0] = Luos_CreateContainer(MotorDC_MsgHandler, MOTOR_TYPE, "DC_motor1_mod", revision);
    container[1] = Luos_CreateContainer(MotorDC_MsgHandler, MOTOR_TYPE, "DC_motor2_mod", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void MotorDC_Loop(void)
{
}
/******************************************************************************
 * @brief Msg manager call back when a msg receive for this container
 * @param Container destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void MotorDC_MsgHandler(container_t *container, msg_t *msg)
{
    if (msg->header.cmd == RATIO)
    {
        // set the motor position
        ratio_t power;
        RatioOD_RatioFromMsg(&power, msg);
        set_power(container, power);
        return;
    }
}

static int find_id(container_t *my_container)
{
    int i = 0;
    for (i = 0; i <= MOTORNUMBER; i++)
    {
        if ((int)my_container == (int)container[i])
            return i;
    }
    return i;
}

static void set_power(container_t *container, ratio_t power)
{
    // limit power value
    if (power < -100.0)
        power = -100.0;
    if (power > 100.0)
        power = 100.0;
    // transform power ratio to timer value
    uint16_t pulse;
    if (power > 0.0)
    {
        pulse = (uint16_t)(power * 50.0);
    }
    else
    {
        pulse = (uint16_t)(-power * 50.0);
    }
    switch (find_id(container))
    {
        case 0:
            if (power > 0.0)
            {
                TIM2->CCR1 = pulse;
                TIM2->CCR2 = 0;
            }
            else
            {
                TIM2->CCR1 = 0;
                TIM2->CCR2 = pulse;
            }
            break;
        case 1:
            if (power > 0.0)
            {
                TIM3->CCR1 = pulse;
                TIM3->CCR2 = 0;
            }
            else
            {
                TIM3->CCR1 = 0;
                TIM3->CCR2 = pulse;
            }
            break;
        default:
            break;
    }
}
