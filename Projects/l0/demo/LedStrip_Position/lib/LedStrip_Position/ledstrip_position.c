/******************************************************************************
 * @file alarm controler
 * @brief application example an alarm controler
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "ledstrip_position.h"
#include "main.h"
#include <math.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef enum
{
    LEDSTRIP_POSITION_APP = LUOS_LAST_TYPE,
} ledstrip_position_t;

#define FRAMERATE_MS       10
#define STRIP_LENGTH       0.43
#define SPACE_BETWEEN_LEDS 0.0068
#define LED_NUMBER         (uint16_t)(STRIP_LENGTH / SPACE_BETWEEN_LEDS)
#define MAXRADIUS          0.05
/*******************************************************************************
 * Variables
 ******************************************************************************/
service_t *app;
uint8_t position = 0;
// distance occupied from sensor variables
linear_position_t distance = -0.001;
// image variables
volatile color_t image[LED_NUMBER];
float radius = 0.0;
/*******************************************************************************
 * Function
 ******************************************************************************/
static void LedStripPosition_MsgHandler(service_t *service, msg_t *msg);
static void distance_filtering(void);
static void frame_compute(void);
static void glowing_fade(float target);
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
    memset((void *)image, 0, LED_NUMBER * sizeof(color_t));
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void LedStripPosition_Loop(void)
{
    static uint32_t lastframe_time_ms = 0;
    static short previous_id          = -1;

    // ********** hot plug management ************
    // Check if we have done the first init or if service Id have changed
    if (previous_id != RoutingTB_IDFromService(app))
    {
        if (RoutingTB_IDFromService(app) == 0)
        {
            // someone is making a detection, let it finish.
            // reset the init state to be ready to setup service at the end of detection
            previous_id = 0;
        }
        else
        {
            // Make services configurations
            // try to find a distance sensor
            int id = RoutingTB_IDFromType(DISTANCE_TYPE);
            if (id > 0)
            {
                // Setup auto update each UPDATE_PERIOD_MS on imu
                // This value is resetted on all service at each detection
                // It's important to setting it each time.
                msg_t msg;
                msg.header.target      = id;
                msg.header.target_mode = IDACK;
                time_luos_t time       = TimeOD_TimeFrom_ms(FRAMERATE_MS);
                TimeOD_TimeToMsg(&time, &msg);
                msg.header.cmd = UPDATE_PUB;
                while (Luos_SendMsg(app, &msg) != SUCCEED)
                {
                    Luos_Loop();
                }
            }
            previous_id = RoutingTB_IDFromService(app);
        }
        return;
    }

    // ********** frame management ************
    // Update the frame
    if (Luos_GetSystick() - lastframe_time_ms >= FRAMERATE_MS)
    {
        int id = RoutingTB_IDFromType(COLOR_TYPE);
        // Check if there is a led_strip detected
        if (id > 0)
        {
            // Check if the distance is in the led strip length
            if ((distance > 0) && (distance < STRIP_LENGTH))
            {
                // Image to light the region of the object detected
                // Compute a frame
                frame_compute();

                // Check in which region there is an object
                if (distance <= (STRIP_LENGTH / 3.0))
                {
                    position = 1;
                }
                else if (distance <= 2 * (STRIP_LENGTH / 3.0))
                {
                    position = 2;
                }
                else if (distance <= STRIP_LENGTH)
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
            msg_t msg;
            msg.header.target_mode = ID;
            msg.header.target      = id;
            msg.header.cmd         = COLOR;
            Luos_SendData(app, &msg, &image[0], sizeof(color_t) * LED_NUMBER);
            // reinitialize the image so that the led_strip is not lighted by default
            memset((void *)image, 0, LED_NUMBER * sizeof(color_t));
        }
        lastframe_time_ms = Luos_GetSystick();
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
        if (distance > STRIP_LENGTH)
        {
            distance = -0.001;
        }
        distance_filtering();
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
        while (Luos_SendMsg(app, &pub_msg) != SUCCEED)
        {
            Luos_Loop();
        }
    }
}

void distance_filtering(void)
{
    // Linear movement
    const float filtering_strength = 0.15;
    const float inertia_strength   = 0.05;
    const float max_speed          = 2.0;

    // Filtering variables
    static linear_position_t prev_distance  = 0.0;
    static linear_position_t inertial_force = 0.0;

    // Clear filter when hand is removed
    if (distance < 0.001)
    {
        distance       = prev_distance;
        inertial_force = 0.0;
        // Glowing fade out
        glowing_fade(0.0);
    }
    // Start filter when hand is present
    else if (radius < 0.00001)
    {
        prev_distance = distance;
        // Glowing fade in
        glowing_fade(MAXRADIUS);
    }
    else
    {
        // Glowing fade in
        glowing_fade(MAXRADIUS);

        // Compute the error between the light and the real hand position
        float position_err = distance - prev_distance;

        // Compute inertial delta force (integral)
        inertial_force += position_err;
        // Inertia clamping
        if (inertial_force < -max_speed)
            inertial_force = -max_speed;
        if (inertial_force > max_speed)
            inertial_force = max_speed;

        // Then filter the position to give an inertia effect
        distance = prev_distance + (filtering_strength * position_err) + (inertia_strength * inertial_force);
    }
    prev_distance = distance;
}

void glowing_fade(float target)
{
    const float filtering_strength = 0.15;
    const float inertia_strength   = 0.08;
    // Radial glowing
    const float max_radius_speed = 1.0;

    // Filtering variables
    static float inertial_force = 0.0;
    // Compute the error between the target and the actual radius
    float radius_err = target - radius;

    // Compute inertial delta force (integral)
    inertial_force += radius_err;
    // Inertia clamping
    if (inertial_force < -max_radius_speed)
        inertial_force = -max_radius_speed;
    if (inertial_force > max_radius_speed)
        inertial_force = max_radius_speed;

    // Then filter the radius to give an inertia effect
    radius = radius + (filtering_strength * radius_err) + (inertia_strength * inertial_force);
    if (radius < 0.0)
    {
        radius = 0.0;
    }
}

void frame_compute(void)
{
    //memset((void *)&image[(uint16_t)(distance / SPACE_BETWEEN_LEDS)], 200, sizeof(color_t));
    const uint16_t radius_led_number = (uint16_t)round((radius) / SPACE_BETWEEN_LEDS) + 1;
    const int max_intensity          = 200;
    uint16_t middle_led              = (uint16_t)(distance / SPACE_BETWEEN_LEDS);
    for (int i = (middle_led - radius_led_number); i < (middle_led + radius_led_number); i++)
    {
        // Conpute the real position in mm of this led
        float real_position = i * SPACE_BETWEEN_LEDS;
        // Parabolic
        //int intensity = max_intensity * (1 - ((real_position - distance) * (real_position - distance) / (radius * radius)));
        // Linear
        int intensity = max_intensity * (1 - (fabs(real_position - distance) / (radius)));
        if ((intensity > 0) && (i < LED_NUMBER) && (i > 0))
        {
            image[i].b = (uint8_t)intensity;
            image[i].g = (uint8_t)intensity;
            image[i].r = (uint8_t)intensity;
        }
    }
}