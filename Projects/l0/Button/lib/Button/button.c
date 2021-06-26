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

container_t *app;
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
    app = Luos_LaunchProfile(&button_profile, "button", revision);
}

/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Button_Loop(void)
{
    button.state = (bool)HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin);

    //Now send a message
    msg_t led_msg;

    //Get the ID of our LED from the routing table
    uint8_t id_led = RoutingTB_IDFromAlias("led");

    led_msg.header.target      = id_led;   //We are sending this to the LED
    led_msg.header.cmd         = IO_STATE; //We are specifying an IO state (on or off)
    led_msg.header.target_mode = IDACK;    //We are asking for an acknowledgement

    led_msg.header.size = sizeof(char); //Our message only contains one character, the IO state
    led_msg.data[0]     = button.state; //The I/O state of the LED to be sent (1 or 0, on or off)
    Luos_SendMsg(app, &led_msg);        //Now that we have the elements, send the message
}
