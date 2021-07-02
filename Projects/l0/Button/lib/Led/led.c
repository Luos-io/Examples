/******************************************************************************
 * @file button
 * @brief driver example a simple button
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "led.h"
#include "gpio.h"

// use profile framework
#include "profile_state.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

// create a general core profile handler
profile_core_t led_profile;
// create an state profile
profile_state_t led;
/*******************************************************************************
 * Function
 ******************************************************************************/

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Led_Init(void)
{
    revision_t revision = {.unmap = REV};

    // Link state profile to the core profile handler
    Luos_LinkProfile(&led_profile, &led, 0);
    // Container creation following template
    Luos_LaunchProfile(&led_profile, "led", revision);
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Led_Loop(void)
{
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, led.state);
}
