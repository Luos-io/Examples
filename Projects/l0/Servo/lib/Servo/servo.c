/******************************************************************************
 * @file servo
 * @brief driver example a simple servo motor
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "main.h"
#include "servo.h"
#include "tim.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define SERVONUMBER 4

typedef struct
{
    angular_position_t angle;
    servo_parameters_t param;
} servo_t;
/*******************************************************************************
 * Variables
 ******************************************************************************/
static container_t *container_serv[SERVONUMBER];
volatile servo_t servo[SERVONUMBER];

/*******************************************************************************
 * Function
 ******************************************************************************/
static void Servo_MsgHandler(container_t *container, msg_t *msg);
static void set_position(uint8_t motor_id);
static uint8_t find_id(container_t *my_container);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Servo_Init(void)
{
    revision_t revision = {.unmap = REV};

    container_serv[0] = Luos_CreateContainer(Servo_MsgHandler, SERVO_TYPE, "servo1_mod", revision);
    container_serv[1] = Luos_CreateContainer(Servo_MsgHandler, SERVO_TYPE, "servo2_mod", revision);
    container_serv[2] = Luos_CreateContainer(Servo_MsgHandler, SERVO_TYPE, "servo3_mod", revision);
    container_serv[3] = Luos_CreateContainer(Servo_MsgHandler, SERVO_TYPE, "servo4_mod", revision);
    servo_parameters_t param;
    param.max_angle      = 180.0;
    param.max_pulse_time = 1.5 / 1000.0;
    param.min_pulse_time = 0.5 / 1000.0;
    servo[0].param       = param;
    servo[0].angle       = 0.0;
    servo[1].param       = param;
    servo[1].angle       = 0.0;
    servo[2].param       = param;
    servo[2].angle       = 0.0;
    servo[3].param       = param;
    servo[3].angle       = 0.0;
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Servo_Loop(void)
{
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this container
 * @param Container destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Servo_MsgHandler(container_t *container, msg_t *msg)
{
    if (msg->header.cmd == ANGULAR_POSITION)
    {
        // set the motor position
        uint8_t motor_id = find_id(container);
        AngularOD_PositionFromMsg((angular_position_t *)&servo[motor_id].angle, msg);
        set_position(motor_id);
        return;
    }
    if (msg->header.cmd == PARAMETERS)
    {
        // set the servo parameters
        uint8_t motor_id = find_id(container);
        memcpy((void *)servo[motor_id].param.unmap, msg->data, sizeof(servo_parameters_t));
        set_position(motor_id);
        return;
    }
}

static void set_position(uint8_t motor_id)
{
    static char chan1 = 0;
    static char chan2 = 0;
    static char chan3 = 0;
    static char chan4 = 0;
    // limit angle value
    if (servo[motor_id].angle < 0.0)
        servo[motor_id].angle = 0.0;
    if (servo[motor_id].angle > servo[motor_id].param.max_angle)
        servo[motor_id].angle = servo[motor_id].param.max_angle;
    // transform angle to timer value
    //const uint32_t min = 400; // min pulse to have 0,5ms
    //const uint32_t max = 800; // max pulse to have 1,5ms
    uint32_t pulse_min = (uint32_t)(servo[motor_id].param.min_pulse_time * (float)(48000000 / htim2.Init.Prescaler));
    uint32_t pulse_max = (uint32_t)(servo[motor_id].param.max_pulse_time * (float)(48000000 / htim2.Init.Prescaler));
    uint32_t pulse     = pulse_min + (uint32_t)(servo[motor_id].angle / servo[motor_id].param.max_angle * (pulse_max - pulse_min));

    switch (motor_id)
    {
        case 0:
            if (!chan1)
            {
                HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
                chan1 = 1;
            }
            TIM2->CCR1 = pulse;
            break;
        case 1:
            if (!chan2)
            {
                HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
                chan2 = 1;
            }
            TIM2->CCR2 = pulse;
            break;
        case 2:
            if (!chan3)
            {
                HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
                chan3 = 1;
            }
            TIM2->CCR3 = pulse;
            break;
        case 3:
            if (!chan4)
            {
                HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
                chan4 = 1;
            }
            TIM2->CCR4 = pulse;
            break;
        default:
            break;
    }
}

static uint8_t find_id(container_t *my_container)
{
    uint8_t i = 0;
    for (i = 0; i <= SERVONUMBER; i++)
    {
        if ((int)my_container == (int)container_serv[i])
            return i;
    }
    return i;
}