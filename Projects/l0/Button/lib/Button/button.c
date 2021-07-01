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
// create an array which will contain all commands
profile_cmd_t button_cmd[NB_CMD];
// create an handler for each command
state_cmd_t button;
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

    CREATE_STATE_PROFILE(button_profile, button_cmd, button, "button", revision)
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Button_Loop(void)
{
    button.value = (bool)HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin);

    Luos_SendProfile("led", "button", IO_STATE, &button, sizeof(state_cmd_t));
}
