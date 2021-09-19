/******************************************************************************
 * @file start controller
 * @brief application example a start controller
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "motor_copy.h"
#include "profile_servo_motor.h"
#include "od_ratio.h"
#include "product_config.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define STARTUP_DELAY_MS       50
#define REFRESH_POSITION_MOTOR 10

#define DEFAULT_ANGULAR_SPEED 180 // 180°/s
/*******************************************************************************
 * Variables
 ******************************************************************************/
static service_t *app;
uint32_t position_refresh = 0;
uint8_t position          = NO_MOTOR;
uint16_t motor_table[3];

/*******************************************************************************
 * Function
 ******************************************************************************/
static void MotorCopy_EventHandler(service_t *service, msg_t *msg);
static void Motor_init(uint16_t id);
static void motor_set(uint8_t motor_target, float position);
static void sort_motors(void);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void MotorCopy_Init(void)
{
    revision_t revision = {.major = 0, .minor = 1, .build = 1};
    // Create App
    app = Luos_CreateService(MotorCopy_EventHandler, COPY_MOTOR, "motor_copy", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void MotorCopy_Loop(void)
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
                int ledstrip_id = RoutingTB_IDFromType(LEDSTRIP_POSITION_APP);
                if (ledstrip_id > 0)
                {
                    msg_t msg;

                    // Switch the LEDSTRIP_POSITION_APP to copy mode
                    msg.header.target_mode = IDACK;
                    msg.header.target      = ledstrip_id;
                    msg.header.cmd         = PARAMETERS;
                    msg.header.size        = 1;
                    msg.data[0]            = MOTOR_COPY_DISPLAY;
                    while (Luos_SendMsg(app, &msg) != SUCCEED)
                    {
                        Luos_Loop();
                    }
                    int id = RoutingTB_IDFromAlias("dxl_2");
                    if (id == 0)
                    {
                        id = RoutingTB_IDFromAlias("dxl_3");
                    }
                    if (id > 0)
                    {
                        // Setup auto update each UPDATE_PERIOD_MS on dxl
                        // This value is resetted on all service at each detection
                        // It's important to setting it each time.
                        msg.header.target      = id;
                        msg.header.target_mode = IDACK;
                        time_luos_t time       = TimeOD_TimeFrom_ms(REFRESH_POSITION_MOTOR);
                        TimeOD_TimeToMsg(&time, &msg);
                        msg.header.cmd = UPDATE_PUB;
                        while (Luos_SendMsg(app, &msg) != SUCCEED)
                        {
                            Luos_Loop();
                        }

                        // Setup dxl
                        servo_motor_mode_t servo_mode = {
                            .mode_compliant = true,
                            // control POWER / ANGULAR POSITION
                            .mode_angular_speed    = false,
                            .mode_angular_position = true,
                            .angular_position      = true};

                        msg.header.target      = id;
                        msg.header.cmd         = PARAMETERS;
                        msg.header.target_mode = IDACK;
                        msg.header.size        = sizeof(servo_motor_mode_t);
                        memcpy(&msg.data, &servo_mode, sizeof(servo_motor_mode_t));
                        while (Luos_SendMsg(app, &msg) != SUCCEED)
                        {
                            Luos_Loop();
                        }

                        // Compute the position of the dxl
                        sort_motors();
                        msg.header.target      = ledstrip_id;
                        msg.header.cmd         = SET_CMD;
                        msg.header.target_mode = IDACK;
                        msg.header.size        = 1;
                        msg.data[0]            = position;
                        while (Luos_SendMsg(app, &msg) != SUCCEED)
                        {
                            Luos_Loop();
                        }
                    }

                    // find the other motors and configure them

                    id = RoutingTB_IDFromAlias("servo_motor");
                    if (id != 0)
                    {
                        Motor_init(id);
                    }
                    id = RoutingTB_IDFromAlias("servo_motor1");
                    if (id != 0)
                    {
                        Motor_init(id);
                    }
                    id = RoutingTB_IDFromAlias("servo_motor2");
                    if (id != 0)
                    {
                        Motor_init(id);
                    }
                    previous_id = RoutingTB_IDFromService(app);
                }
                return;
            }
        }
    }
}

void MotorCopy_EventHandler(service_t *service, msg_t *msg)
{
    if (msg->header.cmd == ANGULAR_POSITION)
    {
        angular_position_t target;
        AngularOD_PositionFromMsg(&target, msg);
        int id = RoutingTB_IDFromAlias("servo_motor");
        if (id != 0)
        {
            motor_set(id, target);
        }
        id = RoutingTB_IDFromAlias("servo_motor1");
        if (id != 0)
        {
            motor_set(id, target);
        }
        id = RoutingTB_IDFromAlias("servo_motor2");
        if (id != 0)
        {
            motor_set(id, target);
        }
    }
}

void Motor_init(uint16_t id)
{
    // send a command to the motor
    servo_motor_mode_t servo_mode = {
        .mode_compliant = false,
        // control POWER / ANGULAR POSITION
        .mode_angular_speed    = false,
        .mode_angular_position = true,
        .angular_position      = false};

    msg_t msg;
    msg.header.target      = id;
    msg.header.cmd         = PARAMETERS;
    msg.header.target_mode = IDACK;
    msg.header.size        = sizeof(servo_motor_mode_t);
    memcpy(&msg.data, &servo_mode, sizeof(servo_motor_mode_t));
    while (Luos_SendMsg(app, &msg) != SUCCEED)
    {
        Luos_Loop();
    }

    float resolution       = 12.0;
    msg.header.target      = id;
    msg.header.cmd         = RESOLUTION;
    msg.header.target_mode = IDACK;
    msg.header.size        = sizeof(float);
    memcpy(&msg.data, &resolution, sizeof(float));
    while (Luos_SendMsg(app, &msg) != SUCCEED)
    {
        Luos_Loop();
    }

    float reduction        = 74.83;
    msg.header.target      = id;
    msg.header.cmd         = REDUCTION;
    msg.header.target_mode = IDACK;
    msg.header.size        = sizeof(float);
    memcpy(&msg.data, &reduction, sizeof(float));
    while (Luos_SendMsg(app, &msg) != SUCCEED)
    {
        Luos_Loop();
    }

    asserv_pid_t pid_coef  = {.p = 28.0, .i = 0.1, .d = 100.0};
    msg.header.target      = id;
    msg.header.cmd         = PID;
    msg.header.target_mode = IDACK;
    msg.header.size        = sizeof(asserv_pid_t);
    memcpy(&msg.data, &pid_coef, sizeof(asserv_pid_t));
    while (Luos_SendMsg(app, &msg) != SUCCEED)
    {
        Luos_Loop();
    }
}

void motor_set(uint8_t motor_target, angular_position_t position)
{
    // send
    msg_t msg;
    msg.header.target      = motor_target;
    msg.header.cmd         = ANGULAR_POSITION;
    msg.header.target_mode = IDACK;
    msg.header.size        = sizeof(angular_position_t);
    memcpy(&msg.data, &position, sizeof(angular_position_t));
    while (Luos_SendMsg(app, &msg) == FAILED)
    {
        Luos_Loop();
    }
}

static void sort_motors(void)
{
    // Parse routing table to find motors
    int motor_found = 0;
    int id          = RoutingTB_IDFromAlias("servo_motor");
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
            position                 = motor_found + 1;
            // Now sort them
            for (int y = motor_found; y > 0; y--)
            {
                if (id < motor_table[y - 1])
                {
                    motor_table[y]     = motor_table[y - 1];
                    motor_table[y - 1] = id;
                    position           = y; // (-1 + 1, because positions start at 1 not 0)
                }
            }
        }
    }
}