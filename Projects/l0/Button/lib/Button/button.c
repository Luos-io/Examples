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

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

// create a general profile handler
profile_t button_1_profile;
profile_state_t button_1;
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

    // Link state profile to the general profile handler
    Luos_LinkProfile(&button_1_profile, &button_1, 0);
    // Container creation following template
    Luos_LaunchProfile(&button_1_profile, "button_1", revision);
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Button_Loop(void)
{
    // button_1.state = (bool)HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin);
    button_1.state = true;
}
