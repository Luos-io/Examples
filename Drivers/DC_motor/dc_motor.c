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
#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s

#define MOTORNUMBER 2
/*******************************************************************************
 * Variables
 ******************************************************************************/
module_t *module[MOTORNUMBER];
/*******************************************************************************
 * Function
 ******************************************************************************/
static void MotorDC_MsgHandler(module_t *module, msg_t *msg);
static int find_id(module_t *my_module);
static void set_power(module_t *module, ratio_t power);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void MotorDC_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    module[0] = Luos_CreateModule(MotorDC_MsgHandler, DCMOTOR_MOD, "DC_motor1_mod", STRINGIFY(VERSION));
    module[1] = Luos_CreateModule(MotorDC_MsgHandler, DCMOTOR_MOD, "DC_motor2_mod", STRINGIFY(VERSION));
    Luos_ModuleEnableRT(module[0]);
    Luos_ModuleEnableRT(module[1]);
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
 * @brief Msg manager call back when a msg receive for this module
 * @param Module destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void MotorDC_MsgHandler(module_t *module, msg_t *msg)
{
    if (msg->header.cmd == RATIO)
    {
        // set the motor position
        ratio_t power;
        RatioOD_RatioFromMsg(&power, msg);
        set_power(module, power);
        return;
    }
}

static int find_id(module_t *my_module)
{
    int i = 0;
    for (i = 0; i <= MOTORNUMBER; i++)
    {
        if ((int)my_module == (int)module[i])
            return i;
    }
    return i;
}

static void set_power(module_t *module, ratio_t power)
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
    switch (find_id(module))
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
