/******************************************************************************
 * @file Power switch
 * @brief driver example a simple Power switch
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "main.h"
#include "power_switch.h"
#include "template_state.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
template_state_t power_switch_template;
profile_state_t *power_switch = &power_switch_template.profile;
/*******************************************************************************
 * Function
 ******************************************************************************/

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void PowerSwitch_Init(void)
{
    revision_t revision = {.Major = 1, .Minor = 0, .Build = 0};
    // Profile configuration
    power_switch->access = WRITE_ONLY_ACCESS;
    TemplateState_CreateContainer(0, &power_switch_template, "power_switch", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void PowerSwitch_Loop(void)
{
    HAL_GPIO_WritePin(GPIOA, SWITCH_Pin, power_switch->state);
}
