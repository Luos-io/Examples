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
#include "string.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifdef REV
revision_t revision = {.unmap = REV};
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile msg_t pub_msg;
volatile int pub = LUOS_PROTOCOL_NB;
volatile dxl_t dxl[MAX_CONTAINER_NUMBER];
volatile unsigned char request_nb = 0;
container_t *my_container[MAX_CONTAINER_NUMBER];
container_t *container_pointer;
uint16_t dxl_table[MAX_CONTAINER_NUMBER];
uint8_t dxl_request_table[MAX_CONTAINER_NUMBER];
dxl_models_t dxl_model[MAX_CONTAINER_NUMBER];
uint16_t position[MAX_CONTAINER_NUMBER] = {0};
uint16_t temperature[MAX_CONTAINER_NUMBER] = {0};
volatile char publish = 0;
/*******************************************************************************
 * Function
 ******************************************************************************/
static void Dxl_MsgHandler(container_t *container, msg_t *msg);
static void discover_dxl(void);
static void dxl_request_manager(void);
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
    static int id = 0;
    static uint32_t last_temp[MAX_CONTAINER_NUMBER] = {0};
    //check motor values one by one
    // Get motor info
    if (dxl_table[id] == 0)
        id = 0;
    if (dxl_table[id] != 0)
    {
        uint16_t tmp_val = 0;
        servo_error_t errors = servo_get_raw_word(dxl_table[id], SERVO_REGISTER_PRESENT_ANGLE, &tmp_val, DXL_TIMEOUT);
        if ((errors != SERVO_ERROR_TIMEOUT) & (errors != SERVO_ERROR_INVALID_RESPONSE))
        {
            position[id] = tmp_val;
        }
        if (HAL_GetTick() - last_temp[id] > TEMP_REFRESH_MS)
        {
            errors = servo_get_raw_word(dxl_table[id], SERVO_REGISTER_PRESENT_TEMPERATURE, &tmp_val, DXL_TIMEOUT);
            if ((errors != SERVO_ERROR_TIMEOUT) & (errors != SERVO_ERROR_INVALID_RESPONSE))
            {
                temperature[id] = tmp_val;
                last_temp[id] = HAL_GetTick();
            }
        }
    }
    id++;
    dxl_request_manager();
}
/******************************************************************************
 * @brief Msg handler call back when a msg receive for this container
 * @param Container destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Dxl_MsgHandler(container_t *container, msg_t *msg)
{
    static unsigned char last = 0;

    if (msg->header.cmd == REGISTER)
    {
        uint16_t reg;
        uint32_t val;
        memcpy(&reg, msg->data, sizeof(uint16_t));
        memcpy(&val, &msg->data[2], (msg->header.size - sizeof(uint16_t)));

        dxl[last].val = (float)val;
        dxl[last].reg = reg;
        dxl[last].container_pointer = container;
        dxl[last].mode = MODE_REG;
        last++;
        if (last == MAX_CONTAINER_NUMBER)
        {
            last = 0;
        }
        request_nb++;
        return;
    }
    if (msg->header.cmd == SETID)
    {
        char id;
        memcpy(&id, msg->data, sizeof(char));
        dxl[last].val = id;
        dxl[last].container_pointer = container;
        dxl[last].mode = MODE_ID;
        last++;
        if (last == MAX_CONTAINER_NUMBER)
        {
            last = 0;
        }
        request_nb++;
        return;
    }
    if (msg->header.cmd == REINIT)
    {
        dxl[last].container_pointer = container;
        dxl[last].mode = MODE_DETECT;
        last++;
        if (last == MAX_CONTAINER_NUMBER)
        {
            last = 0;
        }
        request_nb++;
        return;
    }
    if (msg->header.cmd == ANGULAR_POSITION)
    {
        AngularOD_PositionFromMsg((angular_position_t *)&dxl[last].val, msg);
        dxl[last].container_pointer = container;
        dxl[last].mode = MODE_ANGLE;
        last++;
        if (last == MAX_CONTAINER_NUMBER)
        {
            last = 0;
        }
        request_nb++;
        return;
    }
    if (msg->header.cmd == RATIO_LIMIT)
    {
        float load;
        memcpy(&load, msg->data, sizeof(float));
        dxl[last].val = load;
        dxl[last].container_pointer = container;
        dxl[last].mode = MODE_POWER_LIMIT;
        last++;
        if (last == MAX_CONTAINER_NUMBER)
        {
            last = 0;
        }
        request_nb++;
        return;
    }
    if (msg->header.cmd == PID)
    {
        float fpid[3];
        unsigned char pid[3];
        memcpy(&fpid, msg->data, 3 * sizeof(float));
        for (int i = 0; i < 3; i++)
        {
            if (fpid[i] > 254.0)
                fpid[i] = 254.0;
            if (fpid[i] < 0.0)
                fpid[i] = 0.0;
            pid[i] = (int)fpid[i];
        }
        memcpy((void *)&dxl[last].val, pid, 3 * sizeof(char));
        dxl[last].container_pointer = container;
        dxl[last].mode = MODE_PID;
        last++;
        if (last == MAX_CONTAINER_NUMBER)
        {
            last = 0;
        }
        request_nb++;
        return;
    }
    if (msg->header.cmd == ANGULAR_POSITION_LIMIT)
    {
        angular_position_t angle[2];
        memcpy(&angle, msg->data, 2 * sizeof(float));
        dxl[last].val = angle[0];
        dxl[last].val2 = angle[1];
        dxl[last].container_pointer = container;
        dxl[last].mode = MODE_ANGLE_LIMIT;
        last++;
        if (last == MAX_CONTAINER_NUMBER)
        {
            last = 0;
        }
        request_nb++;
        return;
    }
    if (msg->header.cmd == ANGULAR_SPEED)
    {
        AngularOD_SpeedFromMsg((angular_speed_t *)&dxl[last].val, msg);
        dxl[last].container_pointer = container;
        dxl[last].mode = MODE_SPEED;
        last++;
        if (last == MAX_CONTAINER_NUMBER)
        {
            last = 0;
        }
        request_nb++;
        return;
    }
    if (msg->header.cmd == DXL_WHEELMODE)
    {
        dxl[last].reg = (int)msg->data[0];
        dxl[last].container_pointer = container;
        dxl[last].mode = MODE_WHEEL;
        last++;
        if (last == MAX_CONTAINER_NUMBER)
        {
            last = 0;
        }
        request_nb++;
        return;
    }
    if (msg->header.cmd == COMPLIANT)
    {
        dxl[last].reg = (int)msg->data[0];
        dxl[last].container_pointer = container;
        dxl[last].mode = MODE_COMPLIANT;
        request_nb++;
        last++;
        if (last == MAX_CONTAINER_NUMBER)
        {
            last = 0;
        }
        if ((int)msg->data[0] == 0) // STIFF
        {
            angular_position_t value;
            // convert data into deg
            int i = find_id(container);
            if (dxl_model[i] == AX12 || dxl_model[i] == AX18 || dxl_model[i] == XL320)
            {
                value = AngularOD_PositionFrom_deg(((300.0 * (float)position[i]) / (1024.0 - 1.0)) - (300.0 / 2));
            }
            else
            {
                value = AngularOD_PositionFrom_deg(((360.0 * (float)position[i]) / (4096.0 - 1.0)) - (360.0 / 2));
            }
            dxl[last].val = AngularOD_PositionTo_deg(value);
            dxl[last].container_pointer = container;
            dxl[last].mode = MODE_ANGLE;
            last++;
            if (last == MAX_CONTAINER_NUMBER)
            {
                last = 0;
            }
            request_nb++;
        }
        return;
    }
    if (msg->header.cmd == ASK_PUB_CMD)
    {
        if (!publish)
        {
            pub_msg.header.target = msg->header.source;
            container_pointer = container;
            pub = ASK_PUB_CMD;
            dxl_request_table[find_id(container)] = 1;
        }
        return;
    }
}

static void discover_dxl(void)
{
    int y = 0;
    char my_string[15];
    // Clear container table
    Luos_ContainersClear();
    // Clear local tables
    memset(my_container, 0, sizeof(container_t *) * MAX_CONTAINER_NUMBER);
    memset(dxl_table, 0, sizeof(uint16_t) * MAX_CONTAINER_NUMBER);
    memset(dxl_model, 0, sizeof(dxl_models_t) * MAX_CONTAINER_NUMBER);
    memset((void *)dxl, 0, sizeof(dxl_t) * MAX_CONTAINER_NUMBER);

    HAL_NVIC_DisableIRQ(USART3_4_IRQn);
    HAL_NVIC_SetPriority(USART3_4_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART3_4_IRQn);

    for (int i = 0; i < MAX_ID; i++)
    {
        if (!servo_ping(i, DXL_TIMEOUT))
        {
            // no timeout occured, there is a servo here
            sprintf(my_string, "dxl_%d", i);
            my_container[y] = Luos_CreateContainer(Dxl_MsgHandler, DYNAMIXEL_MOD, my_string, revision);
            dxl_table[y] = i;

            servo_get_raw_word(i, SERVO_REGISTER_MODEL_NUMBER, (uint16_t *)&dxl_model[y], DXL_TIMEOUT);
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
        my_container[y] = Luos_CreateContainer(Dxl_MsgHandler, VOID_MOD, "void_dxl", revision);
    }
}

static void dxl_request_manager(void)
{
    static unsigned char last = 0;
    int i = 0;
    while (request_nb != 0)
    {
        // Send something to a motor
        __disable_irq();
        request_nb--;
        __enable_irq();
        // find the motor id from container_pointer
        i = find_id(dxl[last].container_pointer);
        if (i < MAX_CONTAINER_NUMBER)
        {
            if (dxl[last].mode == MODE_REG)
            {
                if (dxl[last].reg == SERVO_REGISTER_BAUD_RATE)
                {
                    if (dxl[last].container_pointer->ll_container->type == VOID_MOD)
                    {
                        servo_init(57600);
                        HAL_Delay(500);
                        servo_set_raw_byte(SERVO_BROADCAST_ID, SERVO_REGISTER_BAUD_RATE, 1, DXL_TIMEOUT);
                    }
                    else
                    {
                        volatile char baud = 3; // Default value for 1000000 for MX/XL

                        if (dxl_model[i] == AX12 || dxl_model[i] == AX18)
                        {
                            baud = 1; // Default value for 1000000
                            switch ((uint32_t)dxl[last].val)
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
                            switch ((uint32_t)dxl[last].val)
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
                        servo_init((uint32_t)dxl[last].val);
                    }
                }
                if (dxl[last].reg == FACTORY_RESET_REG)
                {
                    // check if it is a void container or not
                    if (dxl[last].container_pointer->ll_container->type == VOID_MOD)
                    {
                        //If it is a void container send it to general call
                        servo_factory_reset(SERVO_BROADCAST_ID, DXL_TIMEOUT);
                    }
                    else
                    {
                        //else to an specific ID
                        servo_factory_reset(dxl_table[i], DXL_TIMEOUT);
                    }
                }
                else
                {
                    char size = get_register_size(dxl[last].reg);
                    if (size == 2)
                    {
                        servo_set_raw_word(dxl_table[i], dxl[last].reg, (uint16_t)dxl[last].val, DXL_TIMEOUT);
                    }
                    else
                    {
                        servo_set_raw_byte(dxl_table[i], dxl[last].reg, (uint8_t)dxl[last].val, DXL_TIMEOUT);
                    }
                }
            }
            if (dxl[last].mode == MODE_ANGLE)
            {
                if (dxl_model[i] == AX12 || dxl_model[i] == AX18 || dxl_model[i] == XL320)
                {
                    int pos = (int)((1024 - 1) * ((300 / 2 + dxl[last].val) / 300));
                    servo_set_raw_word(dxl_table[i], SERVO_REGISTER_GOAL_ANGLE, pos, DXL_TIMEOUT);
                }
                else
                {
                    int pos = (int)((4096 - 1) * ((360 / 2 + dxl[last].val) / 360));
                    servo_set_raw_word(dxl_table[i], SERVO_REGISTER_GOAL_ANGLE, pos, DXL_TIMEOUT);
                }
            }
            if (dxl[last].mode == MODE_ANGLE_LIMIT)
            {
                if (dxl_model[i] == AX12 || dxl_model[i] == AX18 || dxl_model[i] == XL320)
                {
                    int pos = (int)((1024 - 1) * ((300 / 2 + dxl[last].val) / 300));
                    servo_set_raw_word(dxl_table[i], SERVO_REGISTER_MIN_ANGLE, pos, DXL_TIMEOUT);
                    HAL_Delay(1);
                    pos = (int)((1024 - 1) * ((300 / 2 + dxl[last].val2) / 300));
                    servo_set_raw_word(dxl_table[i], SERVO_REGISTER_MAX_ANGLE, pos, DXL_TIMEOUT);
                }
                else
                {
                    int pos = (int)((4096 - 1) * ((360 / 2 + dxl[last].val) / 360));
                    servo_set_raw_word(dxl_table[i], SERVO_REGISTER_MIN_ANGLE, pos, DXL_TIMEOUT);
                    HAL_Delay(1);
                    pos = (int)((4096 - 1) * ((360 / 2 + dxl[last].val2) / 360));
                    servo_set_raw_word(dxl_table[i], SERVO_REGISTER_MAX_ANGLE, pos, DXL_TIMEOUT);
                }
            }
            if (dxl[last].mode == MODE_POWER_LIMIT)
            {
                unsigned short limit = (unsigned short)(dxl[last].val * 1023.0 / 100.0);
                servo_set_raw_word(dxl_table[i], SERVO_REGISTER_TORQUE_LIMIT, limit, DXL_TIMEOUT);
            }
            if (dxl[last].mode == MODE_ID)
            {
                // check address integrity
                if ((int)dxl[last].val < 255)
                {
                    unsigned char id = (unsigned char)dxl[last].val;
                    servo_set_raw_byte(dxl_table[i], SERVO_REGISTER_ID, id, DXL_TIMEOUT);
                    dxl_table[i] = id;
                }
            }
            if (dxl[last].mode == MODE_PID)
            {
                if (dxl_model[i] >= MX12)
                {
                    unsigned char pid[3];
                    memcpy(pid, (void *)&dxl[last].val, 3 * sizeof(char));
                    servo_set_raw_byte(dxl_table[i], SERVO_REGISTER_P_GAIN, pid[0], DXL_TIMEOUT);
                    servo_set_raw_byte(dxl_table[i], SERVO_REGISTER_I_GAIN, pid[1], DXL_TIMEOUT);
                    servo_set_raw_byte(dxl_table[i], SERVO_REGISTER_D_GAIN, pid[2], DXL_TIMEOUT);
                }
            }
            if (dxl[last].mode == MODE_SPEED)
            {
                // Set the direction bit
                int direction = (dxl[last].val < 0) << 10;
                // find the speed factor and compute the max speed
                float speed_factor = 0.111;
                if (dxl_model[i] == MX12 || dxl_model[i] == MX64 || dxl_model[i] == MX106)
                {
                    speed_factor = 0.114;
                }
                float speed_max = 1023.0 * speed_factor * 360.0 / 60.0;
                // Maximisation
                dxl[last].val = fminf(fmaxf(dxl[last].val, -speed_max), speed_max);
                int speed = direction + (int)(fabs(dxl[last].val) / (speed_factor * 360.0 / 60.0));
                servo_set_raw_word(dxl_table[i], SERVO_REGISTER_MOVING_SPEED, speed, DXL_TIMEOUT);
            }
            if (dxl[last].mode == MODE_WHEEL)
            {
                if (dxl[last].reg == 1)
                {
                    if (dxl_model[i] == XL320)
                    {
                        servo_set_raw_byte(dxl_table[i], SERVO_REGISTER_TORQUE_ENABLE, 0, DXL_TIMEOUT);
#ifdef V2
                        servo_set_raw_byte(dxl_table[i], SERVO_REGISTER_CONTROL_MODE, 1, DXL_TIMEOUT);
#endif
                        servo_set_raw_byte(dxl_table[i], SERVO_REGISTER_TORQUE_ENABLE, 1, DXL_TIMEOUT);
                    }
                    else
                    {
                        servo_set_raw_word(dxl_table[i], SERVO_REGISTER_MIN_ANGLE, 0, DXL_TIMEOUT);
                        servo_set_raw_word(dxl_table[i], SERVO_REGISTER_MAX_ANGLE, 0, DXL_TIMEOUT);
                    }
                }
                else
                {
                    servo_set_raw_word(dxl_table[i], SERVO_REGISTER_MIN_ANGLE, 0, DXL_TIMEOUT);
                    if (dxl_model[i] == XL320)
                    {
                        servo_set_raw_byte(dxl_table[i], SERVO_REGISTER_TORQUE_ENABLE, 0, DXL_TIMEOUT);
#ifdef V2
                        servo_set_raw_byte(dxl_table[i], SERVO_REGISTER_CONTROL_MODE, 2, DXL_TIMEOUT);
#endif
                        servo_set_raw_byte(dxl_table[i], SERVO_REGISTER_TORQUE_ENABLE, 1, DXL_TIMEOUT);
                    }
                    else
                    {
                        if (dxl_model[i] == AX12 || dxl_model[i] == AX18)
                        {
                            servo_set_raw_word(dxl_table[i], SERVO_REGISTER_MAX_ANGLE, 1023, DXL_TIMEOUT);
                        }
                        else
                        {
                            servo_set_raw_word(dxl_table[i], SERVO_REGISTER_MAX_ANGLE, 4095, DXL_TIMEOUT);
                        }
                    }
                }
            }
            if (dxl[last].mode == MODE_DETECT)
            {
                discover_dxl();
            }
            if (dxl[last].mode == MODE_COMPLIANT)
            {
                servo_set_raw_byte(dxl_table[i], SERVO_REGISTER_TORQUE_ENABLE, (dxl[last].reg != 1), DXL_TIMEOUT);
            }
        }
        last++;
        if (last == MAX_CONTAINER_NUMBER)
        {
            last = 0;
        }
    }
}

void dxl_publish_manager(void)
{
    publish = 1;
    int i = 0;
    // Send information to the gate
    if (pub == ASK_PUB_CMD)
    {
        // loop into asking motors
        while (dxl_table[i] != 0)
        {
            if (dxl_request_table[i])
            {
                dxl_request_table[i] = 0;
                pub_msg.header.target_mode = ID;
                angular_position_t value;
                // convert data into deg
                if (dxl_model[i] == AX12 || dxl_model[i] == AX18 || dxl_model[i] == XL320)
                {
                    value = ((300.0 * (float)position[i]) / (1024.0 - 1.0)) - (300.0 / 2);
                }
                else
                {
                    value = ((360.0 * (float)position[i]) / (4096.0 - 1.0)) - (360.0 / 2);
                }
                // Send position informations deg
                AngularOD_PositionToMsg(&value, (msg_t *)&pub_msg);
                Luos_SendMsg(my_container[i], (msg_t *)&pub_msg);
                // Send temperature informations °C if there is a value
                if (temperature[i] != 0)
                {
                    temperature_t temp = TemperatureOD_TemperatureFrom_deg_c(temperature[i]);
                    TemperatureOD_TemperatureToMsg(&temp, (msg_t *)&pub_msg);
                    Luos_SendMsg(my_container[i], (msg_t *)&pub_msg);
                    temperature[i] = 0;
                }
            }
            i++;
        }
        pub = LUOS_PROTOCOL_NB;
    }
    publish = 0;
}

static int find_id(container_t *container)
{

    int i = 0;
    for (i = 0; i <= MAX_CONTAINER_NUMBER; i++)
    {
        if ((int)container == (int)my_container[i])
            return i;
    }
    return i;
}
