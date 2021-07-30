/******************************************************************************
 * @file dxl
 * @brief driver example a simple dxl
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "main.h"
#include "dxl.h"
#include "Dynamixel_Servo.h"
#include "math.h"
#include "stdio.h"
#include <float.h>
#include "template_servo_motor.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

typedef struct
{
    profile_servo_motor_t *dxl_motor;
    dxl_models_t model;
    unsigned char id;
} dxl_t;

template_servo_motor_t dxl_motor_template[MAX_CONTAINER_NUMBER];
dxl_t dxl[MAX_CONTAINER_NUMBER];

// volatile char publish = 0;
/*******************************************************************************
 * Function
 ******************************************************************************/
static void Dxl_MsgHandler(container_t *container, msg_t *msg);
static void discover_dxl(void);
static int find_id(container_t *container);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Dxl_Init(void)
{
    servo_init(1000000);
    HAL_Delay(500);
    discover_dxl();
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Dxl_Loop(void)
{
    static int index                                = 0;
    static uint32_t last_temp[MAX_CONTAINER_NUMBER] = {0};
    //check motor values one by one
    // Get motor info
    if (dxl[index].id == 0)
        index = 0;
    if (dxl[index].id != 0)
    {
        uint16_t tmp_val     = 0;
        servo_error_t errors = servo_get_raw_word(dxl[index].id, SERVO_REGISTER_PRESENT_ANGLE, &tmp_val, DXL_TIMEOUT);
        if ((errors != SERVO_ERROR_TIMEOUT) & (errors != SERVO_ERROR_INVALID_RESPONSE))
        {
            if (dxl[index].model == AX12 || dxl[index].model == AX18 || dxl[index].model == XL320)
            {
                dxl[index].dxl_motor->angular_position = AngularOD_PositionFrom_deg(((300.0 * (float)tmp_val) / (1024.0 - 1.0)) - (300.0 / 2));
            }
            else
            {
                dxl[index].dxl_motor->angular_position = AngularOD_PositionFrom_deg(((360.0 * (float)tmp_val) / (4096.0 - 1.0)) - (360.0 / 2));
            }
        }
        if (HAL_GetTick() - last_temp[index] > TEMP_REFRESH_MS)
        {
            errors = servo_get_raw_word(dxl[index].id, SERVO_REGISTER_PRESENT_TEMPERATURE, &tmp_val, DXL_TIMEOUT);
            if ((errors != SERVO_ERROR_TIMEOUT) & (errors != SERVO_ERROR_INVALID_RESPONSE))
            {
                dxl[index].dxl_motor->motor.temperature = TemperatureOD_TemperatureFrom_deg_c(tmp_val);
                last_temp[index]                        = HAL_GetTick();
            }
            // setup the motor to send a one shot temperature each TEMP_REFRESH_MS
            dxl[index].dxl_motor->motor.mode.temperature = dxl[index].dxl_motor->mode.temperature;
        }
    }
    index++;
}
/******************************************************************************
 * @brief Msg handler call back when a msg receive for this container
 * @param Container destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Dxl_MsgHandler(container_t *container, msg_t *msg)
{
    int index = find_id(container);

    if (msg->header.cmd == GET_CMD)
    {
        // if a temperature have been send stop temperature transmission to make it one shot
        dxl[index].dxl_motor->motor.mode.temperature = 0;
    }
    if (msg->header.cmd == REGISTER)
    {
        uint16_t reg;
        uint32_t val;
        memcpy(&reg, msg->data, sizeof(uint16_t));
        memcpy(&val, &msg->data[4], sizeof(uint32_t));

        if (reg == SERVO_REGISTER_BAUD_RATE)
        {
            if (dxl[0].id == 0)
            {
                // we are on a void configuration
                servo_init(57600);
                HAL_Delay(500);
                servo_set_raw_byte(SERVO_BROADCAST_ID, SERVO_REGISTER_BAUD_RATE, 1, DXL_TIMEOUT);
            }
            else
            {
                volatile char baud = 3; // Default value for 1000000 for MX/XL
                if (dxl[index].model == AX12 || dxl[index].model == AX18)
                {
                    baud = 1; // Default value for 1000000
                    switch (val)
                    {
                        case 9600:
                            baud = 207;
                            break;
                        case 19200:
                            baud = 103;
                            break;
                        case 57600:
                            baud = 34;
                            break;
                        case 115200:
                            baud = 16;
                            break;
                        case 200000:
                            baud = 9;
                            break;
                        case 250000:
                            baud = 7;
                            break;
                        case 400000:
                            baud = 4;
                            break;
                        case 500000:
                            baud = 3;
                            break;
                        default:
                            break;
                    }
                }
                else
                {
                    // void container will use this mode
                    switch (val)
                    {
                        case 9600:
                            baud = 0;
                            break;
                        case 57600:
                            baud = 1;
                            break;
                        case 115200:
                            baud = 2;
                            break;
                        default:
                            break;
                    }
                }
                servo_set_raw_byte(SERVO_BROADCAST_ID, SERVO_REGISTER_BAUD_RATE, baud, DXL_TIMEOUT);
                // Set actual baudrate into container
                servo_init(val);
            }
        }
        if (reg == FACTORY_RESET_REG)
        {
            // check if it is a void container or not
            if (dxl[0].id == 0)
            {
                //If it is a void container send it to general call
                servo_factory_reset(SERVO_BROADCAST_ID, DXL_TIMEOUT);
            }
            else
            {
                //else reset a specific ID
                servo_factory_reset(dxl[index].id, DXL_TIMEOUT);
            }
        }
        else
        {
            // This is another kind of register
            char size = get_register_size(reg);
            if (size == 2)
            {
                servo_set_raw_word(dxl[index].id, reg, (uint16_t)val, DXL_TIMEOUT);
            }
            else
            {
                servo_set_raw_byte(dxl[index].id, reg, (uint8_t)val, DXL_TIMEOUT);
            }
        }

        return;
    }
    if (msg->header.cmd == SETID)
    {
        unsigned char id;
        memcpy(&id, msg->data, sizeof(unsigned char));
        // check address integrity
        if (id < 255)
        {
            servo_set_raw_byte(dxl[index].id, SERVO_REGISTER_ID, id, DXL_TIMEOUT);
            dxl[index].id = id;
        }
        return;
    }
    if (msg->header.cmd == REINIT)
    {
        discover_dxl();
        return;
    }
    if (msg->header.cmd == ANGULAR_POSITION)
    {
        int pos;
        if (dxl[index].model == AX12 || dxl[index].model == AX18 || dxl[index].model == XL320)
        {
            pos = (int)((1024 - 1) * ((300 / 2 + dxl[index].dxl_motor->target_angular_position) / 300));
        }
        else
        {
            pos = (int)((4096 - 1) * ((360 / 2 + dxl[index].dxl_motor->target_angular_position) / 360));
        }
        int retry = 0;
        while ((servo_set_raw_word(dxl[index].id, SERVO_REGISTER_GOAL_ANGLE, pos, DXL_TIMEOUT) != SERVO_NO_ERROR) && (retry < 10))
        {
            retry++;
        }
        return;
    }
    if (msg->header.cmd == RATIO_LIMIT)
    {
        unsigned short limit = (unsigned short)(dxl[index].dxl_motor->motor.power * 1023.0 / 100.0);
        servo_set_raw_word(dxl[index].id, SERVO_REGISTER_TORQUE_LIMIT, limit, DXL_TIMEOUT);
        return;
    }
    if (msg->header.cmd == PID)
    {
        if (dxl[index].model >= MX12)
        {
            // clamp PID values
            for (int i = 0; i < 3; i++)
            {
                if (dxl[index].dxl_motor->position_pid.table[i] > 254.0)
                    dxl[index].dxl_motor->position_pid.table[i] = 254.0;
                if (dxl[index].dxl_motor->position_pid.table[i] < 0.0)
                    dxl[index].dxl_motor->position_pid.table[i] = 0.0;
            }
            servo_set_raw_byte(dxl[index].id, SERVO_REGISTER_P_GAIN, (char)dxl[index].dxl_motor->position_pid.p, DXL_TIMEOUT);
            servo_set_raw_byte(dxl[index].id, SERVO_REGISTER_I_GAIN, (char)dxl[index].dxl_motor->position_pid.i, DXL_TIMEOUT);
            servo_set_raw_byte(dxl[index].id, SERVO_REGISTER_D_GAIN, (char)dxl[index].dxl_motor->position_pid.d, DXL_TIMEOUT);
        }
        return;
    }
    if (msg->header.cmd == ANGULAR_POSITION_LIMIT)
    {

        if (dxl[index].model == AX12 || dxl[index].model == AX18 || dxl[index].model == XL320)
        {
            int pos = (int)((1024 - 1) * ((300 / 2 + dxl[index].dxl_motor->limit_angular_position[0]) / 300));
            servo_set_raw_word(dxl[index].id, SERVO_REGISTER_MIN_ANGLE, pos, DXL_TIMEOUT);
            HAL_Delay(1);
            pos = (int)((1024 - 1) * ((300 / 2 + dxl[index].dxl_motor->limit_angular_position[1]) / 300));
            servo_set_raw_word(dxl[index].id, SERVO_REGISTER_MAX_ANGLE, pos, DXL_TIMEOUT);
        }
        else
        {
            int pos = (int)((4096 - 1) * ((360 / 2 + dxl[index].dxl_motor->limit_angular_position[0]) / 360));
            servo_set_raw_word(dxl[index].id, SERVO_REGISTER_MIN_ANGLE, pos, DXL_TIMEOUT);
            HAL_Delay(1);
            pos = (int)((4096 - 1) * ((360 / 2 + dxl[index].dxl_motor->limit_angular_position[1]) / 360));
            servo_set_raw_word(dxl[index].id, SERVO_REGISTER_MAX_ANGLE, pos, DXL_TIMEOUT);
        }
        return;
    }
    if (msg->header.cmd == ANGULAR_SPEED)
    {

        // Set the direction bit
        int direction = (dxl[index].dxl_motor->target_angular_speed < 0) << 10;
        // find the speed factor and compute the max speed
        float speed_factor = 0.111;
        if (dxl[index].model == MX12 || dxl[index].model == MX64 || dxl[index].model == MX106)
        {
            speed_factor = 0.114;
        }
        float speed_max = 1023.0 * speed_factor * 360.0 / 60.0;
        // Maximisation
        dxl[index].dxl_motor->target_angular_speed = fminf(fmaxf(dxl[index].dxl_motor->target_angular_speed, -speed_max), speed_max);
        int speed                                  = direction + (int)(fabs(dxl[index].dxl_motor->target_angular_speed) / (speed_factor * 360.0 / 60.0));
        servo_set_raw_word(dxl[index].id, SERVO_REGISTER_MOVING_SPEED, speed, DXL_TIMEOUT);
        return;
    }
    if (msg->header.cmd == PARAMETERS)
    {
        // Some motors need some time to manage all of those. Just try it multiple times.
        // set compliance
        uint8_t retry = 0;
        while ((servo_set_raw_byte(dxl[index].id, SERVO_REGISTER_TORQUE_ENABLE, (dxl[index].dxl_motor->mode.mode_compliant == 0), DXL_TIMEOUT) != SERVO_NO_ERROR) && (retry < 10))
        {
            retry++;
        }
        if (!dxl[index].dxl_motor->mode.mode_compliant)
        {
            // If we are not compliant set current position as target position
            dxl[index].dxl_motor->target_angular_position = dxl[index].dxl_motor->angular_position;
            int pos;
            if (dxl[index].model == AX12 || dxl[index].model == AX18 || dxl[index].model == XL320)
            {
                pos = (int)((1024 - 1) * ((300 / 2 + dxl[index].dxl_motor->target_angular_position) / 300));
            }
            else
            {
                pos = (int)((4096 - 1) * ((360 / 2 + dxl[index].dxl_motor->target_angular_position) / 360));
            }
            retry = 0;
            while ((servo_set_raw_word(dxl[index].id, SERVO_REGISTER_GOAL_ANGLE, pos, DXL_TIMEOUT) != SERVO_NO_ERROR) && (retry < 10))
            {
                retry++;
            }
        }
        return;
    }
}

static void discover_dxl(void)
{
    revision_t revision = {.major = 1, .minor = 0, .build = 0};
    int y               = 0;
    char alias[15];
    // Clear container table
    Luos_ContainersClear();
    // Clear local tables
    memset((void *)dxl, 0, sizeof(dxl_t) * MAX_CONTAINER_NUMBER);
    for (int i = 0; i < MAX_CONTAINER_NUMBER; i++)
    {
        dxl[i].dxl_motor = &dxl_motor_template[i].profile;
        memset(dxl[i].dxl_motor, 0, sizeof(profile_servo_motor_t));
    }

    HAL_NVIC_DisableIRQ(USART3_4_IRQn);
    HAL_NVIC_SetPriority(USART3_4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_4_IRQn);

    for (int i = 0; i < MAX_ID; i++)
    {
        servo_error_t error;
        error = servo_ping(i, DXL_TIMEOUT);
        if (error)
        {
            //The first try fail, retry
            error = servo_ping(i, DXL_TIMEOUT);
        }
        if (!error)
        {
            // no timeout occured, there is a servo here
            sprintf(alias, "dxl_%d", i);

            // ************** Default configuration settings *****************
            // motor mode by default
            dxl[y].dxl_motor->mode.mode_compliant        = 1;
            dxl[y].dxl_motor->mode.current               = 0;
            dxl[y].dxl_motor->mode.mode_power            = 0;
            dxl[y].dxl_motor->mode.mode_angular_position = 1;
            dxl[y].dxl_motor->mode.mode_angular_speed    = 0;
            dxl[y].dxl_motor->mode.mode_linear_position  = 0;
            dxl[y].dxl_motor->mode.mode_linear_speed     = 0;
            dxl[y].dxl_motor->mode.angular_position      = 1;
            dxl[y].dxl_motor->mode.angular_speed         = 0;
            dxl[y].dxl_motor->mode.linear_position       = 0;
            dxl[y].dxl_motor->mode.linear_speed          = 0;
            dxl[y].dxl_motor->mode.temperature           = 1;

            // default motor configuration
            dxl[y].dxl_motor->wheel_diameter = 0.100f;

            // default motor limits
            dxl[y].dxl_motor->motor.limit_ratio            = 100.0;
            dxl[y].dxl_motor->limit_angular_position[MINI] = -FLT_MAX;
            dxl[y].dxl_motor->limit_angular_position[MAXI] = FLT_MAX;

            dxl[y].id = i;

            // ************** Container creation *****************
            TemplateServoMotor_CreateContainer(Dxl_MsgHandler, &dxl_motor_template[y], alias, revision);
            servo_get_raw_word(i, SERVO_REGISTER_MODEL_NUMBER, (uint16_t *)&dxl[y].model, DXL_TIMEOUT);
            // put a delay on motor response
            servo_set_raw_byte(i, SERVO_REGISTER_RETURN_DELAY_TIME, 10, DXL_TIMEOUT);
            // set limit temperature to 55°C
            servo_set_raw_byte(i, SERVO_REGISTER_MAX_TEMPERATURE, 55, DXL_TIMEOUT);
            y++;
        }
    }
    HAL_NVIC_DisableIRQ(USART3_4_IRQn);
    HAL_NVIC_SetPriority(USART3_4_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(USART3_4_IRQn);
    if (y == 0)
    {
        // there is no motor detected, create a Void container to only manage l0 things
        Luos_CreateContainer(Dxl_MsgHandler, VOID_TYPE, "void_dxl", revision);
    }
}

static int find_id(container_t *container)
{
    int i = 0;
    for (i = 0; i <= MAX_CONTAINER_NUMBER; i++)
    {
        if ((int)container->template_context == (int)&dxl_motor_template[i])
            return i;
    }
    return i;
}
