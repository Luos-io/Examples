/******************************************************************************
 * @file alarm controler
 * @brief application example an alarm controler
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "ledstrip_position.h"
#include "main.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef enum
{
    LEDSTRIP_POSITION_APP = LUOS_LAST_TYPE,
} ledstrip_position_t;

#define LED_NUMBER 75
#define LENGTH     0.55
/*******************************************************************************
 * Variables
 ******************************************************************************/
service_t *app;
uint8_t position = 0;
// distance occupied from sensor variables
linear_position_t distance      = -0.001;
linear_position_t prev_distance = -0.001;
// tick counters for sending messages
volatile uint32_t tickstart_led;
volatile uint32_t tickstart_dist;
volatile uint32_t init_time;
// image table
color_t image[LED_NUMBER];
/*******************************************************************************
 * Function
 ******************************************************************************/
static void LedStripPosition_MsgHandler(service_t *service, msg_t *msg);
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void LedStripPosition_Init(void)
{
    revision_t revision = {.major = 1, .minor = 0, .build = 0};
    // Create App
    app = Luos_CreateService(LedStripPosition_MsgHandler, LEDSTRIP_POSITION_APP, "ledstrip_pos", revision);
    // initialize image to 0
    memset((void *)image, 0, LED_NUMBER * 3);
    // initialize tick counters
    init_time      = 0;
    tickstart_led  = Luos_GetSystick();
    tickstart_dist = Luos_GetSystick();
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void LedStripPosition_Loop(void)
{
    //
    //Distance sensor message - demand the value of distance each 50 ms
    //
    if (RoutingTB_IDFromService(app) > 0)
    {
        int id = RoutingTB_IDFromType(DISTANCE_TYPE);
        // check if there is a sensor detected
        if (id > 0)
        {
            if (Luos_GetSystick() - tickstart_dist >= 50)
            {
                tickstart_dist = Luos_GetSystick();

                msg_t pub_msg;
                pub_msg.header.target_mode = ID;
                pub_msg.header.target      = id;
                pub_msg.header.cmd         = GET_CMD;
                pub_msg.header.size        = 0;
                Luos_SendMsg(app, &pub_msg);
                return;
            }
        }
        //
        // Led strip - change led strip value and define the motor position
        //
        id = RoutingTB_IDFromType(COLOR_TYPE);
        // check if there is a led_strip detected and if the sensor value has changed
        if ((id > 0) && (prev_distance != distance))
        {
            // keep the current sensor value
            prev_distance = distance;
            // check if the distance is in the led strip length
            if (distance > 0)
            {
                if (distance <= LENGTH)
                {
                    // image to light the region of the object detected
                    memset((void *)&image[(uint8_t)(distance * 100)], 200, 20 * sizeof(color_t));
                }
                else
                {
                    // if the distance is longer than the length led strip will note be lightened
                    position = 0;
                }

                //check in which region there is an object
                if (distance <= (LENGTH / 3.0))
                {
                    position = 1;
                }
                else if (distance <= 2 * (LENGTH / 3.0))
                {
                    position = 2;
                }
                else if (distance <= LENGTH)
                {
                    position = 3;
                }
            }
            else
            {
                // no region should be lighted - sensor has not detected sth
                position = 0;
            }
            // send the created image to the led_strip
            if (Luos_GetSystick() - tickstart_led >= 100)
            {
                tickstart_led = Luos_GetSystick();
                msg_t msg;
                msg.header.target_mode = ID;
                msg.header.target      = id;
                msg.header.cmd         = COLOR;
                Luos_SendData(app, &msg, &image[0], sizeof(color_t) * LED_NUMBER);
                // reinitialize the image so that the led_strip is not lighted by default
                memset((void *)image, 0, LED_NUMBER * 3);
            }
        }
    }
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this service
 * @param Service destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void LedStripPosition_MsgHandler(service_t *service, msg_t *msg)
{
    if (RoutingTB_TypeFromID(msg->header.source) == DISTANCE_TYPE)
    {
        // receive the distance sensor value
        LinearOD_PositionFromMsg(&distance, msg);
        //fiter distances with small difference
        if (((distance - prev_distance <= 0.015) || (prev_distance - distance <= 0.015)) && (distance > 0.0) && (prev_distance > 0.0))
        {
            distance = (distance + prev_distance) / 2.0;
        }
    }
    else if (msg->header.cmd == GET_CMD)
    {
        // motor application asks which position of the led_strip is lightened - respond
        msg_t pub_msg;
        pub_msg.header.target_mode = ID;
        pub_msg.header.target      = msg->header.source;
        pub_msg.header.cmd         = SET_CMD;
        pub_msg.header.size        = 1;
        pub_msg.data[0]            = position;
        Luos_SendMsg(app, &pub_msg);
    }
}
