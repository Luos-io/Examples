/******************************************************************************
 * @file Handy
 * @brief driver example a simple handy
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "main.h"
#include "handy.h"
#include "tim.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define STRINGIFY(s) STRINGIFY1(s)
#define STRINGIFY1(s) #s
/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Function
 ******************************************************************************/
static void Handy_MsgHandler(module_t *module, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Handy_Init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    Luos_ModuleEnableRT(Luos_CreateModule(Handy_MsgHandler, HANDY_MOD, "handy_mod", STRINGIFY(VERSION)));
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Handy_Loop(void)
{
}
/******************************************************************************
 * @brief Msg handler call back when a msg receive for this module
 * @param Module destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Handy_MsgHandler(module_t *module, msg_t *msg)
{
    if (msg->header.cmd == HANDY_SET_POSITION)
    {
        // set the motors position
        handy_t position;
        memcpy(position.unmap, msg->data, sizeof(handy_t));
        set_position(&position);
        return;
    }
}
void set_position(handy_t *position)
{
    uint32_t min = (uint32_t)(0.001 * (float)(48000000 / htim2.Init.Prescaler));
    uint32_t max = (uint32_t)(0.002 * (float)(48000000 / htim2.Init.Prescaler));
    const float factor = 1.2;
    uint32_t pulse = min;
    float angle = 0.0;
    if (position->index <= 100)
    {
        // limit angle value
        if (position->index > 100)
            position->index = 100;
        angle = (float)position->index * factor;
        // transform angle to timer value
        pulse = min + (uint16_t)(angle / 120.0 * (max - min));
        TIM2->CCR1 = pulse;
    }
    if (position->middle <= 100)
    {
        // limit angle value
        if (position->middle > 100)
            position->middle = 100;
        angle = (float)position->middle * factor;
        // transform angle to timer value
        pulse = min + (uint16_t)(angle / 120.0 * (max - min));
        TIM2->CCR2 = pulse;
    }
    if (position->ring <= 100)
    {
        // limit angle value
        if (position->ring > 100)
            position->ring = 100;
        angle = (float)position->ring * factor;
        // transform angle to timer value
        pulse = min + (uint16_t)(angle / 120.0 * (max - min));
        TIM2->CCR3 = pulse;
    }
    if (position->pinky <= 100)
    {
        // limit angle value
        if (position->pinky > 100)
            position->pinky = 100;
        angle = (float)position->pinky * factor;
        // transform angle to timer value
        pulse = min + (uint16_t)(angle / 120.0 * (max - min));
        TIM2->CCR4 = pulse;
    }
    if (position->thumb <= 100)
    {
        // limit angle value
        if (position->thumb > 100)
            position->thumb = 100;
        angle = (float)position->thumb * factor;
        // transform angle to timer value
        pulse = min + (uint16_t)(angle / 120.0 * (max - min));
        TIM3->CCR2 = pulse;
    }
}




