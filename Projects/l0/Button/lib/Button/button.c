/******************************************************************************
 * @file button
 * @brief driver example a simple button
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "button.h"
#include "gpio.h"
#include "template_state.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
template_state_t button_template;
profile_state_t *button = &button_template.profile;
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
    revision_t revision = {.major = 1, .minor = 0, .build = 0};
    // Profile configuration
    button->access = READ_ONLY_ACCESS;
    // Service creation following template
    TemplateState_CreateService(0, &button_template, "button", revision);
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Button_Loop(void)
{
    button->state = (bool)HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin);
}
