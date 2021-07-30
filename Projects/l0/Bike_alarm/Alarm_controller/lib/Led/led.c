/******************************************************************************
 * @file Led
 * @brief driver example a simple Led
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "main.h"
#include "led.h"
#include <math.h>
#include "tim.h"
#include "string.h"
#include "luos.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile time_luos_t time;
volatile color_t target_rgb;
volatile color_t last_rgb;
volatile float coef[3] = {0.0};
static int elapsed_ms  = 0;

/*******************************************************************************
 * Function
 ******************************************************************************/
static void Led_MsgHandler(service_t *service, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Led_Init(void)
{
    revision_t revision = {.unmap = REV};
    time                = TimeOD_TimeFrom_ms(0.0);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    Luos_CreateService(Led_MsgHandler, COLOR_TYPE, "rgb_led", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Led_Loop(void)
{
}
/******************************************************************************
 * @brief Msg manager callback when a msg receive for this service
 * @param Service destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Led_MsgHandler(service_t *service, msg_t *msg)
{
    if (msg->header.cmd == COLOR)
    {
        // change led target color
        memcpy((color_t *)&last_rgb, (color_t *)&target_rgb, sizeof(color_t));
        IlluminanceOD_ColorFromMsg((color_t *)&target_rgb, msg);
        if (TimeOD_TimeTo_ms(time) > 0.0)
        {
            elapsed_ms = 0;
            coef[0]    = (float)(target_rgb.r - last_rgb.r) / TimeOD_TimeTo_ms(time);
            coef[1]    = (float)(target_rgb.g - last_rgb.g) / TimeOD_TimeTo_ms(time);
            coef[2]    = (float)(target_rgb.b - last_rgb.b) / TimeOD_TimeTo_ms(time);
        }
        return;
    }
    if (msg->header.cmd == TIME)
    {
        // save transition time
        TimeOD_TimeFromMsg((time_luos_t *)&time, msg);
        return;
    }
}

void pwm_setvalue(color_t *rgb)
{
    TIM3->CCR1 = (uint16_t)rgb->r * 10;
    TIM3->CCR2 = (uint16_t)rgb->g * 10;
    TIM2->CCR1 = (uint16_t)rgb->b * 10;
}

void HAL_SYSTICK_Callback(void)
{
    // ************* motion planning *************
    if (time != 0)
    {
        static color_t rgb;
        if ((float)elapsed_ms > TimeOD_TimeTo_ms(time))
        {
            // we finished our transition
            pwm_setvalue((color_t *)&target_rgb);
            memcpy((color_t *)&last_rgb, (color_t *)&target_rgb, sizeof(color_t));
        }
        else
        {
            rgb.r = (int)(coef[0] * (float)elapsed_ms + (float)last_rgb.r);
            rgb.g = (int)(coef[1] * (float)elapsed_ms + (float)last_rgb.g);
            rgb.b = (int)(coef[2] * (float)elapsed_ms + (float)last_rgb.b);
            elapsed_ms++;
            pwm_setvalue(&rgb);
        }
    }
    else
    {
        pwm_setvalue((color_t *)&target_rgb);
    }
}
