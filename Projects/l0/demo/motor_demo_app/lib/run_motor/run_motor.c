/******************************************************************************
 * @file start controller
 * @brief application example a start controller
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "run_motor.h"
#include "profile_servo_motor.h"
#include "od_ratio.h"
#include "product_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define MOTOR_ID_OFFSET 5

enum
{
    LOWER_BOUND_POSITION  = -90,
    HIGHER_BOUND_POSITION = 90
};

#define REFRESH_POSITION_MOTOR  100
#define REFRESH_DIRECTION_MOTOR 1000

#define DEFAULT_ANGULAR_SPEED 180 // 180°/s
/*******************************************************************************
 * Variables
 ******************************************************************************/
static service_t *app;
static uint8_t current_motor_target = NO_MOTOR;
static uint8_t next_motor_target    = NO_MOTOR;
uint32_t position_refresh           = 0;
uint32_t command_refresh            = 0;
uint16_t led_app_id                 = 0;

float target_position[3] = {
    LOWER_BOUND_POSITION,
    LOWER_BOUND_POSITION,
    LOWER_BOUND_POSITION};

/*******************************************************************************
 * Function
 ******************************************************************************/
void RunMotor_EventHandler(service_t *service, msg_t *msg);
void motor_set(uint8_t motor_target, float position);
void reset_unselected_motor(uint8_t motor_target);
void run_selected_motor(uint8_t motor_target);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void RunMotor_Init(void)
{
    revision_t revision = {.major = 0, .minor = 1, .build = 1};
    // Create App
    app = Luos_CreateService(RunMotor_EventHandler, RUN_MOTOR, "run_motor", revision);

    position_refresh = Luos_GetSystick();
    command_refresh  = Luos_GetSystick();
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void RunMotor_Loop(void)
{
    // ask for the motor's position to move
    if (Luos_GetSystick() - position_refresh > REFRESH_POSITION_MOTOR)
    {
        // reset position_refresh
        position_refresh = Luos_GetSystick();

        led_app_id = RoutingTB_IDFromAlias("ledstrip_pos");
        if (led_app_id != 0)
        {
            // send message to led strip controller
            msg_t msg_led_strip;
            msg_led_strip.header.target      = led_app_id;
            msg_led_strip.header.cmd         = GET_CMD;
            msg_led_strip.header.target_mode = IDACK;
            msg_led_strip.header.size        = 0;
            Luos_SendMsg(app, &msg_led_strip);
        }
    }

    // ask for the current motor's angle position
    if ((Luos_GetSystick() - command_refresh > REFRESH_DIRECTION_MOTOR))
    {
        // reset command_refresh
        command_refresh = Luos_GetSystick();

        // if new target has been received, update selected motor
        // and reset unselected motor to their default position
        if (next_motor_target != current_motor_target)
        {
            current_motor_target = next_motor_target;
            reset_unselected_motor(current_motor_target);
        }

        // update target position if a valid motor is selected
        if (current_motor_target != NO_MOTOR)
        {
            target_position[current_motor_target - 1] = -target_position[current_motor_target - 1];
        }

        // send command to selected motor
        run_selected_motor(current_motor_target);
    }
}

void RunMotor_EventHandler(service_t *service, msg_t *msg)
{
    if (msg->header.source == RoutingTB_IDFromAlias("ledstrip_pos"))
    {
        next_motor_target = msg->data[0];
    }
}

void reset_unselected_motor(uint8_t motor_target)
{
    switch (motor_target)
    {
        case MOTOR_1_POSITION:
            motor_set(MOTOR_2_POSITION + MOTOR_ID_OFFSET, 0);
            motor_set(MOTOR_3_POSITION + MOTOR_ID_OFFSET, 0);
            break;
        case MOTOR_2_POSITION:
            motor_set(MOTOR_1_POSITION + MOTOR_ID_OFFSET, 0);
            motor_set(MOTOR_3_POSITION + MOTOR_ID_OFFSET, 0);
            break;
        case MOTOR_3_POSITION:
            motor_set(MOTOR_1_POSITION + MOTOR_ID_OFFSET, 0);
            motor_set(MOTOR_2_POSITION + MOTOR_ID_OFFSET, 0);
            break;
        default:
            motor_set(MOTOR_1_POSITION + MOTOR_ID_OFFSET, 0);
            motor_set(MOTOR_2_POSITION + MOTOR_ID_OFFSET, 0);
            motor_set(MOTOR_3_POSITION + MOTOR_ID_OFFSET, 0);
            break;
    }
}

void run_selected_motor(uint8_t motor_target)
{
    switch (motor_target)
    {
        case MOTOR_1_POSITION:
            motor_set(MOTOR_1_POSITION + MOTOR_ID_OFFSET, target_position[current_motor_target - 1]);
            break;
        case MOTOR_2_POSITION:
            motor_set(MOTOR_2_POSITION + MOTOR_ID_OFFSET, target_position[current_motor_target - 1]);
            break;
        case MOTOR_3_POSITION:
            motor_set(MOTOR_3_POSITION + MOTOR_ID_OFFSET, target_position[current_motor_target - 1]);
            break;
        default:
            break;
    }
}

void motor_set(uint8_t motor_target, float position)
{
    // send a command to the motor
    servo_motor_mode_t servo_mode = {
        .mode_compliant = false,
        // control POWER / ANGULAR POSITION
        .mode_angular_speed    = true,
        .mode_angular_position = true,
        .angular_position      = true};

    msg_t msg;
    msg.header.target      = motor_target;
    msg.header.cmd         = PARAMETERS;
    msg.header.target_mode = IDACK;
    msg.header.size        = sizeof(servo_motor_mode_t);
    memcpy(&msg.data, &servo_mode, sizeof(servo_motor_mode_t));
    Luos_SendMsg(app, &msg);

    // send angular speed message
    angular_speed_t angular_speed = DEFAULT_ANGULAR_SPEED;
    msg.header.target             = motor_target;
    msg.header.cmd                = ANGULAR_SPEED;
    msg.header.target_mode        = IDACK;
    msg.header.size               = sizeof(angular_speed_t);
    memcpy(&msg.data, &angular_speed, sizeof(angular_speed_t));
    Luos_SendMsg(app, &msg);

    // send
    angular_position_t angular_position = position;
    msg.header.target                   = motor_target;
    msg.header.cmd                      = ANGULAR_POSITION;
    msg.header.target_mode              = IDACK;
    msg.header.size                     = sizeof(angular_position_t);
    memcpy(&msg.data, &angular_position, sizeof(angular_position_t));
    Luos_SendMsg(app, &msg);
}