/******************************************************************************
 * @file button
 * @brief driver example a simple button
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "button.h"
#include "gpio.h"

// use profile framework
#include "profile_state.h"
#include "profile_motor.h"
#include "profile_voltage.h"
#include "profile_servo_motor.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

// create a general core profile handler
profile_core_t button_profile;
profile_core_t voltage_profile;
profile_core_t motor_profile;
profile_core_t servo_motor_profile;
// create an state profile
profile_state_t button;
profile_voltage_t voltage;
profile_motor_t motor;
profile_servo_motor_t servo_motor;
/*******************************************************************************
 * Function
 ******************************************************************************/

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Button_Init(void)
{
    revision_t revision = {.unmap = REV};

    // Link state profile to the core profile handler
    Luos_LinkStateProfile(&button_profile, &button, 0);
    // Container creation following template
    Luos_LaunchProfile(&button_profile, "button", revision);

    Luos_LinkVoltageProfile(&voltage_profile, &voltage, 0);
    Luos_LaunchProfile(&voltage_profile, "voltage", revision);

    Luos_LinkMotorProfile(&motor_profile, &motor, 0);
    Luos_LaunchProfile(&motor_profile, "motor", revision);

    Luos_LinkServoMotorProfile(&servo_motor_profile, &servo_motor, 0);
    Luos_LaunchProfile(&servo_motor_profile, "servo_motor", revision);
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Button_Loop(void)
{
    button.state = (bool)HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin);

    Luos_SendProfile("led", "button", IO_STATE, &button.state, sizeof(bool));
}
