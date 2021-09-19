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

#include "math.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define STARTUP_DELAY_MS 50

#define REFRESH_POSITION_MOTOR  100
#define REFRESH_DIRECTION_MOTOR 100 // in milliseconds

#define MAX_ANGLE 90

#define NB_POINT_IN_TRAJECTORY 100
#define TRAJECTORY_PERIOD      2.0 // in seconds
#define SAMPLING_PERIOD        (float)(TRAJECTORY_PERIOD / NB_POINT_IN_TRAJECTORY)
/*******************************************************************************
 * Variables
 ******************************************************************************/
static service_t *app;
static uint8_t current_motor_target = NO_MOTOR;
static uint8_t next_motor_target    = NO_MOTOR;
uint32_t command_refresh            = 0;
uint16_t led_app_id                 = 0;
uint16_t motor_table[3];
int motor_found = 0;

// Trajectory management
uint32_t trajectory_refresh = 0;
float trajectory[NB_POINT_IN_TRAJECTORY];

/*******************************************************************************
 * Function
 ******************************************************************************/
static void RunMotor_EventHandler(service_t *service, msg_t *msg);
static void motor_init(uint8_t motor_target);
static void motor_SendTrajectory(uint8_t motor_target);
static void motor_stream(uint8_t motor_target, control_type_t control);
static void sort_motors(void);
static void compute_trajectory(void);

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

    command_refresh    = Luos_GetSystick();
    trajectory_refresh = Luos_GetSystick();

    compute_trajectory();
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void RunMotor_Loop(void)
{
    static short previous_id       = -1;
    static uint32_t detection_date = 0;

    // ********** hot plug management ************
    // Check if we have done the first init or if service Id have changed
    if (previous_id != RoutingTB_IDFromService(app))
    {
        if (RoutingTB_IDFromService(app) == 0)
        {
            // someone is making a detection, let it finish.
            // reset the init state to be ready to setup service at the end of detection
            previous_id    = -1;
            detection_date = Luos_GetSystick();
        }
        else
        {
            if ((Luos_GetSystick() - detection_date) > STARTUP_DELAY_MS)
            {
                // A detection just finished
                // Make services configurations
                // ask for the motor's position to move
                int id = RoutingTB_IDFromType(LEDSTRIP_POSITION_APP);
                if (id > 0)
                {
                    // Setup auto update each UPDATE_PERIOD_MS on imu
                    // This value is resetted on all service at each detection
                    // It's important to setting it each time.
                    msg_t msg;
                    msg.header.target      = id;
                    msg.header.target_mode = IDACK;
                    time_luos_t time       = TimeOD_TimeFrom_ms(REFRESH_POSITION_MOTOR);
                    TimeOD_TimeToMsg(&time, &msg);
                    msg.header.cmd = UPDATE_PUB;
                    while (Luos_SendMsg(app, &msg) != SUCCEED)
                    {
                        Luos_Loop();
                    }
                }
                // init motor on the first run
                sort_motors();
                for (int i = 0; i < motor_found; i++)
                {
                    motor_init(motor_table[i]);
                }
                previous_id = RoutingTB_IDFromService(app);
            }
        }
        return;
    }
    // check if we need to change the selected motor
    // if new target has been received, update selected motor
    // and reset unselected motor to their default position
    if (next_motor_target != current_motor_target)
    {
        // send play command to selected motor
        motor_stream(motor_table[current_motor_target - 1], PAUSE);
        if (next_motor_target != NO_MOTOR)
        {
            motor_stream(motor_table[next_motor_target - 1], PLAY);
        }
        current_motor_target = next_motor_target;
    }

    // send trajectory data at a fixed period
    if ((Luos_GetSystick() - trajectory_refresh > (uint32_t)TRAJECTORY_PERIOD * 1000))
    {
        // reset command_refresh
        trajectory_refresh = Luos_GetSystick();

        if (current_motor_target != NO_MOTOR)
        {
            // send trajectory to the current motor
            motor_SendTrajectory(motor_table[current_motor_target - 1]);
        }
    }
}

void RunMotor_EventHandler(service_t *service, msg_t *msg)
{
    if (msg->header.cmd == SET_CMD)
    {
        next_motor_target = msg->data[0];
    }
}

void motor_init(uint8_t motor_target)
{
    msg_t msg;
    // Do not send motor configuration to dxl
    if (strcmp(RoutingTB_AliasFromId(motor_target), "dxl_2") || strcmp(RoutingTB_AliasFromId(motor_target), "dxl_3"))
    {
        // Send sensor resolution
        float resolution       = 12.0;
        msg.header.target      = motor_target;
        msg.header.cmd         = RESOLUTION;
        msg.header.target_mode = IDACK;
        msg.header.size        = sizeof(float);
        memcpy(&msg.data, &resolution, sizeof(float));
        while (Luos_SendMsg(app, &msg) != SUCCEED)
        {
            Luos_Loop();
        }
        // Send reduction ratio resolution
        float reduction        = 74.83;
        msg.header.target      = motor_target;
        msg.header.cmd         = REDUCTION;
        msg.header.target_mode = IDACK;
        msg.header.size        = sizeof(float);
        memcpy(&msg.data, &reduction, sizeof(float));
        while (Luos_SendMsg(app, &msg) != SUCCEED)
        {
            Luos_Loop();
        }
        // Send PID
        asserv_pid_t pid_coef  = {.p = 28.0, .i = 0.1, .d = 100.0};
        msg.header.target      = motor_target;
        msg.header.cmd         = PID;
        msg.header.target_mode = IDACK;
        msg.header.size        = sizeof(asserv_pid_t);
        memcpy(&msg.data, &pid_coef, sizeof(asserv_pid_t));
        while (Luos_SendMsg(app, &msg) != SUCCEED)
        {
            Luos_Loop();
        }
    }
    // Send parameters to the motor
    servo_motor_mode_t servo_mode = {
        .mode_compliant        = false,
        .mode_angular_speed    = false,
        .mode_angular_position = true,
        .angular_position      = true};

    msg.header.target      = motor_target;
    msg.header.cmd         = PARAMETERS;
    msg.header.target_mode = IDACK;
    msg.header.size        = sizeof(servo_motor_mode_t);
    memcpy(&msg.data, &servo_mode, sizeof(servo_motor_mode_t));
    while (Luos_SendMsg(app, &msg) != SUCCEED)
    {
        Luos_Loop();
    }
    // Send sampling frequency
    float sampling_freq    = SAMPLING_PERIOD;
    msg.header.target      = motor_target;
    msg.header.cmd         = TIME;
    msg.header.target_mode = IDACK;
    msg.header.size        = sizeof(float);
    memcpy(&msg.data, &sampling_freq, sizeof(float));
    while (Luos_SendMsg(app, &msg) != SUCCEED)
    {
        Luos_Loop();
    }

    // stop streaming
    motor_stream(motor_target, STOP);

    // send trajectory data
    motor_SendTrajectory(motor_target);
}

void motor_SendTrajectory(uint8_t motor_target)
{
    if (motor_target != 0)
    {
        msg_t msg;
        // send data
        msg.header.target      = motor_target;
        msg.header.cmd         = ANGULAR_POSITION;
        msg.header.target_mode = IDACK;
        msg.header.size        = NB_POINT_IN_TRAJECTORY * sizeof(angular_position_t);
        Luos_SendData(app, &msg, trajectory, NB_POINT_IN_TRAJECTORY * sizeof(angular_position_t));
    }
}

void motor_stream(uint8_t motor_target, control_type_t control_type)
{
    msg_t msg;
    // stop streaming
    control_t control      = {.flux = control_type};
    msg.header.target      = motor_target;
    msg.header.cmd         = CONTROL;
    msg.header.target_mode = IDACK;
    msg.header.size        = NB_POINT_IN_TRAJECTORY * sizeof(angular_position_t);
    ControlOD_ControlToMsg(&control, &msg);
    while (Luos_SendMsg(app, &msg) == FAILED)
    {
        Luos_Loop();
    }
}

static void sort_motors(void)
{
    // Parse routing table to find motors
    int id      = RoutingTB_IDFromAlias("servo_motor");
    motor_found = 0;
    if (id != 0)
    {
        motor_table[motor_found] = id;
        motor_found++;
    }
    id = RoutingTB_IDFromAlias("servo_motor1");
    if (id != 0)
    {
        motor_table[motor_found] = id;
        motor_found++;
    }
    id = RoutingTB_IDFromAlias("servo_motor2");
    if (id != 0)
    {
        motor_table[motor_found] = id;
        motor_found++;
    }
    id = RoutingTB_IDFromAlias("servo_motor3");
    if (id != 0)
    {
        motor_table[motor_found] = id;
        motor_found++;
    }
    if (motor_found < 3)
    {
        // Then get the dxl
        id = RoutingTB_IDFromAlias("dxl_2");
        if (id == 0)
        {
            id = RoutingTB_IDFromAlias("dxl_3");
        }
        if (id != 0)
        {
            motor_table[motor_found] = id;
            // Now sort them
            for (int y = motor_found; y > 0; y--)
            {
                if (id < motor_table[y - 1])
                {
                    motor_table[y]     = motor_table[y - 1];
                    motor_table[y - 1] = id;
                }
            }
            motor_found++;
        }
    }
}

void compute_trajectory(void)
{
    uint32_t index  = 0;
    float angle_deg = 0;
    float quantum   = 360.0 / NB_POINT_IN_TRAJECTORY;

    for (index = 0; index < NB_POINT_IN_TRAJECTORY; index++)
    {
        // compute linear trajectory
        angle_deg = index * quantum;
        // compute sinus trajectory
        trajectory[index] = sin(M_PI / 180 * angle_deg) * MAX_ANGLE;
    }
}