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
profile_t button_profile;
profile_state_t button;
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
    // Profile configuration
    button_profile.access = READ_ONLY_ACCESS;
    // Link state profile to the general profile handler
    Luos_LinkProfile(&button_profile, &button, 0, "button_mod", revision);
    // Container creation following template
    Luos_LaunchProfile(&button_profile);
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Button_Loop(void)
{
    button.state = (bool)HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin);
}
