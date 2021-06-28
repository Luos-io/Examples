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

// create a general core profile handler
profile_core_t button_profile;
// create a specific state profile handler
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

    // Link state profile to the core profile handler
    Luos_LinkProfile(&button_profile, &button, 0);
    // Container creation following template
    Luos_LaunchProfile(&button_profile, "button", revision);
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Button_Loop(void)
{
    button.state = (bool)HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin);

    Luos_SendProfile("led", "button", IO_STATE, &button, sizeof(profile_state_t));
}
