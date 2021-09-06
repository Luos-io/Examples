/******************************************************************************
 * @file start controller
 * @brief application example a start controller
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "detection_button.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define UPDATE_PERIOD_MS 20

typedef enum
{
    LEDSTRIP_POSITION_APP = LUOS_LAST_TYPE,
    DETECTION_BUTTON_APP,
} alarm_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/
service_t *app;
uint8_t detection_enable = 0;
uint8_t reset            = 0;
int previous_id          = -1;
uint8_t last_btn_state   = 0;

/*******************************************************************************
 * Function
 ******************************************************************************/
static void DetectionButton_MsgHandler(service_t *service, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void DetectionButton_Init(void)
{
    revision_t revision = {.major = 1, .minor = 0, .build = 0};
    // Create App
    app = Luos_CreateService(DetectionButton_MsgHandler, DETECTION_BUTTON_APP, "detection_button", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void DetectionButton_Loop(void)
{
    if (RoutingTB_IDFromService(app) == 0)
    {
        // We don't have any ID, meaning no detection occure or detection is occuring.
        if (previous_id == -1)
        {
            // This is the start period, we have to make a detection.
            // Be sure the network is powered up 20 ms before starting a detection
            if (Luos_GetSystick() > 200)
            {
                // No detection occured, do it
                RoutingTB_DetectServices(app);
                previous_id = 0;
                // variable to reinit auto update
                reset = 1;
            }
        }
    }
    else
    {
        // button is pressed so we launch a detection
        if (detection_enable)
        {
            RoutingTB_DetectServices(app);
            //previous_id      = 0;
            reset            = 1;
            detection_enable = 0;
        }
    }
    // if a detection has occured we need to reinitialize the auto update of button
    if (reset)
    {
        int id = RoutingTB_IDFromAlias("button");
        if (id > 0)
        {
            msg_t msg;
            msg.header.target      = id;
            msg.header.target_mode = IDACK;
            // Setup auto update each UPDATE_PERIOD_MS on button
            // This value is resetted on all service at each detection
            // It's important to setting it each time.
            time_luos_t time = TimeOD_TimeFrom_ms(UPDATE_PERIOD_MS);
            TimeOD_TimeToMsg(&time, &msg);
            msg.header.cmd = UPDATE_PUB;
            Luos_SendMsg(app, &msg);
            reset = 0;
        }
    }
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this service
 * @param Service destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void DetectionButton_MsgHandler(service_t *service, msg_t *msg)
{
    if (msg->header.source == RoutingTB_IDFromType(STATE_TYPE))
    {
        // change the detection enable value only if the state of button is different
        if ((!last_btn_state) & (last_btn_state != msg->data[0]))
        {
            detection_enable = (!detection_enable);
        }
        last_btn_state = msg->data[0];
    }
}
