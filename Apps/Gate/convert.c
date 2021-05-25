/******************************************************************************
 * @file convert
 * @brief Functions allowing to manage JSON convertion
 * @author Luos
 ******************************************************************************/
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "convert.h"
#include "luos_utils.h"
#include "gate_config.h"
#include "pipe_link.h"
#include "data_manager.h"
#include "cJSON.h"

static void Convert_SplitFloat(float value, int32_t *integerPart, uint32_t *decimalPart);
static void Convert_JsonToMsg(container_t *service, uint16_t id, luos_type_t type, cJSON *jobj, msg_t *msg, char *bin_data);

/*******************************************************************************
 * Tools
 ******************************************************************************/
// This function reduce Float to string convertion without FPU to 1/3 of normal time.
// This function have been inspired by Benoit Blanchon Blog : https://blog.benoitblanchon.fr/lightweight-float-to-string/
void Convert_SplitFloat(float value, int32_t *integerPart, uint32_t *decimalPart)
{
    float remainder;
    *integerPart = (int32_t)value;
    if (value >= 0.0)
    {
        remainder = (value - *integerPart) * 1000.0;
    }
    else
    {
        remainder = (-(value - *integerPart)) * 1000.0;
    }

    *decimalPart = (uint32_t)remainder;

    // rounding values
    // remainder -= *decimalPart;
    // if (remainder >= 0.5)
    // {
    //     (*decimalPart)++;
    // }
}

/*******************************************************************************
 * Luos Json data to Luos messages convertion
 ******************************************************************************/
// Convert a Json into messages
void Convert_DataToLuos(container_t *service, char *data)
{
    msg_t msg;

    // check if we have a complete received command
    cJSON *root = cJSON_Parse(data);
    // check json integrity
    if (root == NULL)
    {
        // Error
        cJSON_Delete(root);
        return;
    }
    // check if this is a detection cmd
    if (cJSON_GetObjectItem(root, "detection") != NULL)
    {
        detection_ask++;
        cJSON_Delete(root);
        return;
    }
    if (cJSON_GetObjectItem(root, "baudrate") != NULL)
    {
        //create a message to setup the new baudrate
        cJSON *object = cJSON_GetObjectItem(root, "baudrate");
        if (cJSON_IsNumber(object))
        {
            uint32_t baudrate = (float)object->valueint;
            Luos_SendBaudrate(service, baudrate);
        }
        cJSON_Delete(root);
        return;
    }
    if (cJSON_GetObjectItem(root, "benchmark") != NULL)
    {
        //manage benchmark
        if (cJSON_IsObject(cJSON_GetObjectItem(root, "benchmark")))
        {
            // Get all parameters
            cJSON *parameters   = cJSON_GetObjectItem(root, "benchmark");
            uint32_t repetition = 0;
            if (cJSON_IsNumber(cJSON_GetObjectItem(parameters, "repetitions")))
            {
                repetition = (int)cJSON_GetObjectItem(parameters, "repetitions")->valueint;
            }
            uint32_t target_id = (int)cJSON_GetObjectItem(parameters, "target")->valueint;
            cJSON *item        = cJSON_GetObjectItem(parameters, "data");
            uint32_t size      = (int)cJSON_GetArrayItem(item, 0)->valueint;
            if (size > 0)
            {
                // find the first \r of the current buf
                int index = 0;
                for (index = 0; index < GATE_BUFF_SIZE; index++)
                {
                    if (data[index] == '\r')
                    {
                        index++;
                        break;
                    }
                }
                if (index < GATE_BUFF_SIZE - 1)
                {
                    // stop sensor polling during benchmark
                    update_time = 0.0;
                    DataManager_collect(service);
                    // create a message from parameters
                    msg.header.cmd         = REVISION;
                    msg.header.target_mode = IDACK;
                    msg.header.target      = target_id;
                    // save current time
                    uint32_t begin_systick = Luos_GetSystick();
                    uint32_t failed_msg_nb = 0;
                    // Before trying to send anything make sure to finish any transmission
                    while (Luos_TxComplete() == FAILED)
                        ;
                    // Wait 10ms allowing receiving messages to finish
                    uint32_t tickstart = Luos_GetSystick();
                    while ((Luos_GetSystick() - tickstart) < 10)
                        ;
                    // Flush every messages pending
                    Luos_Flush();
                    // To get the number of message failed we will use statistics
                    // We have to reinit the number of dropped message before start
                    uint8_t drop_back                                = service->node_statistics->memory.msg_drop_number;
                    service->node_statistics->memory.msg_drop_number = 0;
                    uint8_t retry_back                               = *service->ll_container->ll_stat.max_retry;
                    *service->ll_container->ll_stat.max_retry        = 0;
                    // send this message multiple time
                    int i = 0;
                    for (i = 0; i < repetition; i++)
                    {
                        Luos_SendData(service, &msg, &data[index], (unsigned int)size);
                    }
                    // Wait transmission end
                    while (Luos_TxComplete() == FAILED)
                        ;
                    // Get the number of failures on transmission
                    failed_msg_nb = service->node_statistics->memory.msg_drop_number;
                    // Get the number of retry
                    // If retry == max retry number consider all messages as lost
                    if (*service->ll_container->ll_stat.max_retry >= NBR_RETRY)
                    {
                        // We failed to transmit this message count all as failed
                        failed_msg_nb = repetition;
                    }
                    service->node_statistics->memory.msg_drop_number = drop_back;
                    *service->ll_container->ll_stat.max_retry        = retry_back;
                    uint32_t end_systick                             = Luos_GetSystick();
                    float data_rate                                  = (float)size * (float)(repetition - failed_msg_nb) / (((float)end_systick - (float)begin_systick) / 1000.0) * 8;
                    float fail_rate                                  = (float)failed_msg_nb * 100.0 / (float)repetition;
                    char tx_json[512];
                    int32_t dataIntegerPart;
                    uint32_t dataDecimalPart;
                    int32_t failIntegerPart;
                    uint32_t failDecimalPart;

                    Convert_SplitFloat(data_rate, &dataIntegerPart, &dataDecimalPart);
                    Convert_SplitFloat(fail_rate, &failIntegerPart, &failDecimalPart);
                    sprintf(tx_json, "{\"benchmark\":{\"data_rate\":%" PRId32 ".%" PRIu32 ",\"fail_rate\":%" PRId32 ".%" PRIu32 "}}\n", dataIntegerPart, dataDecimalPart, failIntegerPart, failDecimalPart);
                    PipeLink_Send(service, tx_json, strlen(tx_json));
                    // restart sensor polling
                    update_time = GATE_REFRESH_TIME_S;
                    DataManager_collect(service);
                }
            }
        }
        cJSON_Delete(root);
        return;
    }
    cJSON *containers = cJSON_GetObjectItem(root, "containers");
    // Get containers
    if (cJSON_IsObject(containers))
    {
        // Loop into containers
        cJSON *container_jsn = containers->child;
        while (container_jsn != NULL)
        {
            // Create msg
            char *alias = container_jsn->string;
            uint16_t id = RoutingTB_IDFromAlias(alias);
            if (id == 65535)
            {
                // If alias doesn't exist in our list id_from_alias send us back -1 = 65535
                // So here there is an error in alias.
                cJSON_Delete(root);
                break;
            }
            luos_type_t type = RoutingTB_TypeFromID(id);
            Convert_JsonToMsg(service, id, type, container_jsn, &msg, (char *)data);
            // Get next container
            container_jsn = container_jsn->next;
        }
        cJSON_Delete(root);
        return;
    }
}
// Create msg from a container json data
void Convert_JsonToMsg(container_t *service, uint16_t id, luos_type_t type, cJSON *jobj, msg_t *msg, char *bin_data)
{
    time_luos_t time;
    float data = 0.0;
    cJSON *item;
    msg->header.target_mode = IDACK;
    msg->header.target      = id;
    //********** global convertion***********
    // ratio
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "power_ratio")))
    {
        ratio_t ratio = (ratio_t)cJSON_GetObjectItem(jobj, "power_ratio")->valuedouble;
        RatioOD_RatioToMsg(&ratio, msg);
        Luos_SendMsg(service, msg);
    }
    // target angular position
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "target_rot_position")))
    {
        angular_position_t angular_position = (angular_position_t)cJSON_GetObjectItem(jobj, "target_rot_position")->valuedouble;
        AngularOD_PositionToMsg(&angular_position, msg);
        Luos_SendMsg(service, msg);
    }
    if (cJSON_IsArray(cJSON_GetObjectItem(jobj, "target_rot_position")))
    {
        int i = 0;
        // this is a trajectory
        item     = cJSON_GetObjectItem(jobj, "target_rot_position");
        int size = (int)cJSON_GetArrayItem(item, 0)->valueint;
        // find the first \r of the current buf
        for (i = 0; i < GATE_BUFF_SIZE; i++)
        {
            if (bin_data[i] == '\r')
            {
                i++;
                break;
            }
        }
        if (i < GATE_BUFF_SIZE - 1)
        {
            msg->header.cmd = ANGULAR_POSITION;
            Luos_SendData(service, msg, &bin_data[i], (unsigned int)size);
        }
    }
    // Limit angular position
    if (cJSON_IsArray(cJSON_GetObjectItem(jobj, "limit_rot_position")))
    {
        angular_position_t limits[2];
        item      = cJSON_GetObjectItem(jobj, "limit_rot_position");
        limits[0] = AngularOD_PositionFrom_deg(cJSON_GetArrayItem(item, 0)->valuedouble);
        limits[1] = AngularOD_PositionFrom_deg(cJSON_GetArrayItem(item, 1)->valuedouble);
        memcpy(&msg->data[0], limits, 2 * sizeof(float));
        msg->header.cmd  = ANGULAR_POSITION_LIMIT;
        msg->header.size = 2 * sizeof(float);
        Luos_SendMsg(service, msg);
    }
    // Limit linear position
    if (cJSON_IsArray(cJSON_GetObjectItem(jobj, "limit_trans_position")))
    {
        linear_position_t limits[2];
        item      = cJSON_GetObjectItem(jobj, "limit_trans_position");
        limits[0] = LinearOD_PositionFrom_mm((float)cJSON_GetArrayItem(item, 0)->valuedouble);
        limits[1] = LinearOD_PositionFrom_mm((float)cJSON_GetArrayItem(item, 1)->valuedouble);
        memcpy(&msg->data[0], limits, 2 * sizeof(linear_position_t));
        msg->header.cmd  = LINEAR_POSITION_LIMIT;
        msg->header.size = 2 * sizeof(linear_position_t);
        Luos_SendMsg(service, msg);
    }
    // Limit angular speed
    if (cJSON_IsArray(cJSON_GetObjectItem(jobj, "limit_rot_speed")))
    {
        angular_speed_t limits[2];
        item      = cJSON_GetObjectItem(jobj, "limit_rot_speed");
        limits[0] = AngularOD_SpeedFrom_deg_s(cJSON_GetArrayItem(item, 0)->valuedouble);
        limits[1] = AngularOD_SpeedFrom_deg_s(cJSON_GetArrayItem(item, 1)->valuedouble);
        memcpy(&msg->data[0], limits, 2 * sizeof(float));
        msg->header.cmd  = ANGULAR_SPEED_LIMIT;
        msg->header.size = 2 * sizeof(float);
        Luos_SendMsg(service, msg);
    }
    // Limit linear speed
    if (cJSON_IsArray(cJSON_GetObjectItem(jobj, "limit_trans_speed")))
    {
        linear_speed_t limits[2];
        item      = cJSON_GetObjectItem(jobj, "limit_trans_speed");
        limits[0] = LinearOD_Speedfrom_mm_s((float)cJSON_GetArrayItem(item, 0)->valuedouble);
        limits[1] = LinearOD_Speedfrom_mm_s((float)cJSON_GetArrayItem(item, 1)->valuedouble);
        memcpy(&msg->data[0], limits, 2 * sizeof(linear_speed_t));
        msg->header.cmd  = LINEAR_SPEED_LIMIT;
        msg->header.size = 2 * sizeof(linear_speed_t);
        Luos_SendMsg(service, msg);
    }
    // Limit ratio
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "limit_power")))
    {
        data = (float)cJSON_GetObjectItem(jobj, "limit_power")->valuedouble;
        memcpy(msg->data, &data, sizeof(data));
        msg->header.cmd  = RATIO_LIMIT;
        msg->header.size = sizeof(float);
        Luos_SendMsg(service, msg);
    }
    // Limit current
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "limit_current")))
    {
        current_t current = (current_t)cJSON_GetObjectItem(jobj, "limit_current")->valuedouble;
        ElectricOD_CurrentToMsg(&current, msg);
        Luos_SendMsg(service, msg);
    }
    // target Rotation speed
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "target_rot_speed")))
    {
        // this should be a function because it is frequently used
        angular_speed_t angular_speed = (angular_speed_t)cJSON_GetObjectItem(jobj, "target_rot_speed")->valuedouble;
        AngularOD_SpeedToMsg(&angular_speed, msg);
        Luos_SendMsg(service, msg);
    }
    // target linear position
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "target_trans_position")))
    {
        linear_position_t linear_position = LinearOD_PositionFrom_mm((float)cJSON_GetObjectItem(jobj, "target_trans_position")->valuedouble);
        LinearOD_PositionToMsg(&linear_position, msg);
        Luos_SendMsg(service, msg);
    }
    if (cJSON_IsArray(cJSON_GetObjectItem(jobj, "target_trans_position")))
    {
        int i = 0;
        // this is a trajectory
        item     = cJSON_GetObjectItem(jobj, "target_trans_position");
        int size = (int)cJSON_GetArrayItem(item, 0)->valueint;
        // find the first \r of the current buf
        for (i = 0; i < GATE_BUFF_SIZE; i++)
        {
            if (bin_data[i] == '\r')
            {
                i++;
                break;
            }
        }
        if (i < GATE_BUFF_SIZE - 1)
        {
            msg->header.cmd = LINEAR_POSITION;
            // todo WATCHOUT this could be mm !
            Luos_SendData(service, msg, &bin_data[i], (unsigned int)size);
        }
    }
    // target Linear speed
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "target_trans_speed")))
    {
        linear_speed_t linear_speed = LinearOD_Speedfrom_mm_s((float)cJSON_GetObjectItem(jobj, "target_trans_speed")->valuedouble);
        LinearOD_SpeedToMsg(&linear_speed, msg);
        Luos_SendMsg(service, msg);
    }
    // time
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "time")))
    {
        // this should be a function because it is frequently used
        time = TimeOD_TimeFrom_s((float)cJSON_GetObjectItem(jobj, "time")->valuedouble);
        TimeOD_TimeToMsg(&time, msg);
        Luos_SendMsg(service, msg);
    }
    // Compliance
    if (cJSON_IsBool(cJSON_GetObjectItem(jobj, "compliant")))
    {
        msg->data[0]     = cJSON_IsTrue(cJSON_GetObjectItem(jobj, "compliant"));
        msg->header.cmd  = COMPLIANT;
        msg->header.size = sizeof(char);
        Luos_SendMsg(service, msg);
    }
    // Pid
    if (cJSON_IsArray(cJSON_GetObjectItem(jobj, "pid")))
    {
        float pid[3];
        item   = cJSON_GetObjectItem(jobj, "pid");
        pid[0] = (float)cJSON_GetArrayItem(item, 0)->valuedouble;
        pid[1] = (float)cJSON_GetArrayItem(item, 1)->valuedouble;
        pid[2] = (float)cJSON_GetArrayItem(item, 2)->valuedouble;
        memcpy(&msg->data[0], pid, sizeof(asserv_pid_t));
        msg->header.cmd  = PID;
        msg->header.size = sizeof(asserv_pid_t);
        Luos_SendMsg(service, msg);
    }
    // resolution
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "resolution")))
    {
        // this should be a function because it is frequently used
        data = (float)cJSON_GetObjectItem(jobj, "resolution")->valuedouble;
        memcpy(msg->data, &data, sizeof(data));
        msg->header.cmd  = RESOLUTION;
        msg->header.size = sizeof(data);
        Luos_SendMsg(service, msg);
    }
    //offset
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "offset")))
    {
        // this should be a function because it is frequently used
        data = (float)cJSON_GetObjectItem(jobj, "offset")->valuedouble;
        memcpy(msg->data, &data, sizeof(data));
        msg->header.cmd  = OFFSET;
        msg->header.size = sizeof(data);
        Luos_SendMsg(service, msg);
    }
    // reduction ratio
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "reduction")))
    {
        // this should be a function because it is frequently used
        data = (float)cJSON_GetObjectItem(jobj, "reduction")->valuedouble;
        memcpy(msg->data, &data, sizeof(data));
        msg->header.cmd  = REDUCTION;
        msg->header.size = sizeof(data);
        Luos_SendMsg(service, msg);
    }
    // dimension (m)
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "dimension")))
    {
        linear_position_t linear_position = LinearOD_PositionFrom_mm((float)cJSON_GetObjectItem(jobj, "dimension")->valuedouble);
        LinearOD_PositionToMsg(&linear_position, msg);
        // redefine a specific message type.
        msg->header.cmd = DIMENSION;
        Luos_SendMsg(service, msg);
    }
    // voltage
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "volt")))
    {
        // this should be a function because it is frequently used
        voltage_t volt = (voltage_t)cJSON_GetObjectItem(jobj, "volt")->valuedouble;
        ElectricOD_VoltageToMsg(&volt, msg);
        Luos_SendMsg(service, msg);
    }
    // reinit
    if (cJSON_GetObjectItem(jobj, "reinit"))
    {
        msg->header.cmd  = REINIT;
        msg->header.size = 0;
        Luos_SendMsg(service, msg);
    }
    // control (play, pause, stop, rec)
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "control")))
    {
        msg->data[0]     = cJSON_GetObjectItem(jobj, "control")->valueint;
        msg->header.cmd  = CONTROL;
        msg->header.size = sizeof(control_mode_t);
        Luos_SendMsg(service, msg);
    }
    // Color
    if (cJSON_IsArray(cJSON_GetObjectItem(jobj, "color")))
    {
        item     = cJSON_GetObjectItem(jobj, "color");
        int size = cJSON_GetArraySize(item);
        if (size == 3)
        {
            color_t color;
            for (int i = 0; i < 3; i++)
            {
                color.unmap[i] = (char)cJSON_GetArrayItem(item, i)->valueint;
            }
            IlluminanceOD_ColorToMsg(&color, msg);
            Luos_SendMsg(service, msg);
        }
        else
        {
            int i = 0;
            // This is a binary
            size = (int)cJSON_GetArrayItem(item, 0)->valueint;
            // find the first \r of the current buf
            for (i = 0; i < GATE_BUFF_SIZE; i++)
            {
                if (bin_data[i] == '\r')
                {
                    i++;
                    break;
                }
            }
            if (i < GATE_BUFF_SIZE - 1)
            {
                msg->header.cmd = COLOR;
                Luos_SendData(service, msg, &bin_data[i], (unsigned int)size);
            }
        }
    }
    // IO_STATE
    if (cJSON_IsBool(cJSON_GetObjectItem(jobj, "io_state")))
    {
        msg->data[0]     = cJSON_IsTrue(cJSON_GetObjectItem(jobj, "io_state"));
        msg->header.cmd  = IO_STATE;
        msg->header.size = sizeof(char);
        Luos_SendMsg(service, msg);
    }
    // update time
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "update_time")) & (type != GATE_MOD))
    {
        // this should be a function because it is frequently used
        time = TimeOD_TimeFrom_s((float)cJSON_GetObjectItem(jobj, "update_time")->valuedouble);
        TimeOD_TimeToMsg(&time, msg);
        msg->header.cmd = UPDATE_PUB;
        Luos_SendMsg(service, msg);
    }
    // UUID
    if (cJSON_GetObjectItem(jobj, "uuid"))
    {
        msg->header.cmd  = NODE_UUID;
        msg->header.size = 0;
        Luos_SendMsg(service, msg);
    }
    // RENAMING
    if (cJSON_IsString(cJSON_GetObjectItem(jobj, "rename")))
    {
        // In this case we need to send the message as system message
        int i            = 0;
        char *alias      = cJSON_GetStringValue(cJSON_GetObjectItem(jobj, "rename"));
        msg->header.size = strlen(alias);
        // Change size to fit into 16 characters
        if (msg->header.size > 15)
        {
            msg->header.size = 15;
        }
        // Clean the '\0' even if we short the alias
        alias[msg->header.size] = '\0';
        // Copy the alias into the data field of the message
        for (i = 0; i < msg->header.size; i++)
        {
            msg->data[i] = alias[i];
        }
        msg->data[msg->header.size] = '\0';
        msg->header.cmd             = WRITE_ALIAS;
        Luos_SendMsg(service, msg);
    }
    // FIRMWARE REVISION
    if (cJSON_GetObjectItem(jobj, "revision"))
    {
        msg->header.cmd  = REVISION;
        msg->header.size = 0;
        Luos_SendMsg(service, msg);
    }
    // Luos REVISION
    if (cJSON_GetObjectItem(jobj, "luos_revision"))
    {
        msg->header.cmd  = LUOS_REVISION;
        msg->header.size = 0;
        Luos_SendMsg(service, msg);
    }
    // Luos STAT
    if (cJSON_GetObjectItem(jobj, "luos_statistics"))
    {
        msg->header.cmd  = LUOS_STATISTICS;
        msg->header.size = 0;
        Luos_SendMsg(service, msg);
    }
    // Parameters
    if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "parameters")))
    {
        uint32_t val = cJSON_GetObjectItem(jobj, "parameters")->valueint;
        memcpy(msg->data, &val, sizeof(uint32_t));
        msg->header.size = 4;
        msg->header.cmd  = PARAMETERS;
        Luos_SendMsg(service, msg);
    }
    if (cJSON_IsArray(cJSON_GetObjectItem(jobj, "parameters")))
    {
        item     = cJSON_GetObjectItem(jobj, "parameters");
        int size = cJSON_GetArraySize(item);
        // find the first \r of the current buf
        for (int i = 0; i < size; i++)
        {
            uint32_t val = (uint32_t)cJSON_GetArrayItem(item, i)->valuedouble;
            memcpy(&msg->data[i * sizeof(uint32_t)], &val, sizeof(uint32_t));
        }
        msg->header.cmd  = PARAMETERS;
        msg->header.size = size * sizeof(uint32_t);
        Luos_SendMsg(service, msg);
    }
    switch (type)
    {
        case VOID_MOD:
        case DYNAMIXEL_MOD:
            if (cJSON_IsArray(cJSON_GetObjectItem(jobj, "register")))
            {
                item    = cJSON_GetObjectItem(jobj, "register");
                int val = (int)cJSON_GetArrayItem(item, 0)->valueint;
                memcpy(&msg->data[0], &val, sizeof(uint16_t));
                val = (int)cJSON_GetArrayItem(item, 1)->valueint;
                if (val <= 0xFF)
                {
                    memcpy(&msg->data[2], &val, sizeof(uint8_t));
                    msg->header.size = sizeof(uint16_t) + sizeof(uint8_t);
                }
                else if (val <= 0xFFFF)
                {
                    memcpy(&msg->data[2], &val, sizeof(uint16_t));
                    msg->header.size = sizeof(uint16_t) + sizeof(uint16_t);
                }
                else
                {
                    memcpy(&msg->data[2], &val, sizeof(uint32_t));
                    msg->header.size = sizeof(uint16_t) + sizeof(uint32_t);
                }
                msg->header.cmd = REGISTER;
                Luos_SendMsg(service, msg);
            }
            if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "set_id")))
            {
                msg->data[0]     = (char)(cJSON_GetObjectItem(jobj, "set_id")->valueint);
                msg->header.cmd  = SETID;
                msg->header.size = sizeof(char);
                Luos_SendMsg(service, msg);
            }
            if (cJSON_IsBool(cJSON_GetObjectItem(jobj, "wheel_mode")))
            {
                msg->data[0]     = cJSON_IsTrue(cJSON_GetObjectItem(jobj, "wheel_mode"));
                msg->header.cmd  = DXL_WHEELMODE;
                msg->header.size = sizeof(char);
                Luos_SendMsg(service, msg);
            }
            break;
            break;
        case GATE_MOD:
            if (cJSON_IsNumber(cJSON_GetObjectItem(jobj, "update_time")))
            {
                // Put all services with the same time value
                update_time = TimeOD_TimeFrom_s((float)cJSON_GetObjectItem(jobj, "update_time")->valuedouble);
                DataManager_collect(service);
            }
            break;
        default:
            break;
    }
}

/*******************************************************************************
 * Luos service information to Json convertion
 ******************************************************************************/
// This function start a Json structure and return the string size.
void Convert_StartData(char *data)
{
    memcpy(data, "{\"containers\":{", sizeof("{\"containers\":{"));
    data += (sizeof("{\"containers\":{") - 1);
}
// This function start a Service into a Json structure and return the string size.
void Convert_StartServiceData(char *data, char *alias)
{
    sprintf(data, "\"%s\":{", alias);
    data += strlen(data);
}
// This function create the Json content from a message and return the string size.
void Convert_MsgToData(msg_t *msg, char *data)
{
    float fdata;
    int32_t integerPart;
    uint32_t decimalPart;
    switch (msg->header.cmd)
    {
        case LINEAR_POSITION:
            memcpy(&fdata, msg->data, msg->header.size);
            Convert_SplitFloat(fdata, &integerPart, &decimalPart);
            sprintf(data, "\"trans_position\":%" PRId32 ".%" PRIu32 ",", integerPart, decimalPart);
            break;
        case LINEAR_SPEED:
            memcpy(&fdata, msg->data, msg->header.size);
            Convert_SplitFloat(fdata, &integerPart, &decimalPart);
            sprintf(data, "\"trans_speed\":%" PRId32 ".%" PRIu32 ",", integerPart, decimalPart);
            break;
        case ANGULAR_POSITION:
            memcpy(&fdata, msg->data, msg->header.size);
            Convert_SplitFloat(fdata, &integerPart, &decimalPart);
            sprintf(data, "\"rot_position\":%" PRId32 ".%" PRIu32 ",", integerPart, decimalPart);
            break;
        case ANGULAR_SPEED:
            memcpy(&fdata, msg->data, msg->header.size);
            Convert_SplitFloat(fdata, &integerPart, &decimalPart);
            sprintf(data, "\"rot_speed\":%" PRId32 ".%" PRIu32 ",", integerPart, decimalPart);
            break;
        case CURRENT:
            memcpy(&fdata, msg->data, msg->header.size);
            Convert_SplitFloat(fdata, &integerPart, &decimalPart);
            sprintf(data, "\"current\":%" PRId32 ".%" PRIu32 ",", integerPart, decimalPart);
            break;
        case ILLUMINANCE:
            memcpy(&fdata, msg->data, msg->header.size);
            Convert_SplitFloat(fdata, &integerPart, &decimalPart);
            sprintf(data, "\"lux\":%" PRId32 ".%" PRIu32 ",", integerPart, decimalPart);
            break;
        case TEMPERATURE:
            memcpy(&fdata, msg->data, msg->header.size);
            Convert_SplitFloat(fdata, &integerPart, &decimalPart);
            sprintf(data, "\"temperature\":%" PRId32 ".%" PRIu32 ",", integerPart, decimalPart);
            break;
        case FORCE:
            memcpy(&fdata, msg->data, msg->header.size);
            Convert_SplitFloat(fdata, &integerPart, &decimalPart);
            sprintf(data, "\"force\":%" PRId32 ".%" PRIu32 ",", integerPart, decimalPart);
            break;
        case MOMENT:
            memcpy(&fdata, msg->data, msg->header.size);
            Convert_SplitFloat(fdata, &integerPart, &decimalPart);
            sprintf(data, "\"moment\":%" PRId32 ".%" PRIu32 ",", integerPart, decimalPart);
            break;
        case VOLTAGE:
            memcpy(&fdata, msg->data, msg->header.size);
            Convert_SplitFloat(fdata, &integerPart, &decimalPart);
            sprintf(data, "\"volt\":%" PRId32 ".%" PRIu32 ",", integerPart, decimalPart);
            break;
        case POWER:
            memcpy(&fdata, msg->data, msg->header.size);
            Convert_SplitFloat(fdata, &integerPart, &decimalPart);
            sprintf(data, "\"power\":%" PRId32 ".%" PRIu32 ",", integerPart, decimalPart);
            break;
        case NODE_UUID:
            if (msg->header.size == sizeof(luos_uuid_t))
            {
                luos_uuid_t value;
                memcpy(value.unmap, msg->data, msg->header.size);
                sprintf(data, "\"uuid\":[%" PRIu32 ",%" PRIu32 ",%" PRIu32 "],", value.uuid[0], value.uuid[1], value.uuid[2]);
            }
            break;
        case REVISION:
            // clean data to be used as string
            if (msg->header.size < MAX_DATA_MSG_SIZE)
            {
                //create the Json content
                sprintf(data, "\"revision\":\"%d.%d.%d\",", msg->data[0], msg->data[1], msg->data[2]);
            }
            break;
        case LUOS_REVISION:
            // clean data to be used as string
            if (msg->header.size < MAX_DATA_MSG_SIZE)
            {
                //create the Json content
                sprintf(data, "\"luos_revision\":\"%d.%d.%d\",", msg->data[0], msg->data[1], msg->data[2]);
            }
            break;
        case LUOS_STATISTICS:
            if (msg->header.size == sizeof(general_stats_t))
            {
                general_stats_t *stat = (general_stats_t *)msg->data;
                // create the Json content
                sprintf(data, "\"luos_statistics\":{\"rx_msg_stack\":%d,\"luos_stack\":%d,\"tx_msg_stack\":%d,\"buffer_occupation\":%d,\"msg_drop\":%d,\"loop_ms\":%d,\"max_retry\":%d},",
                        stat->node_stat.memory.rx_msg_stack_ratio,
                        stat->node_stat.memory.luos_stack_ratio,
                        stat->node_stat.memory.tx_msg_stack_ratio,
                        stat->node_stat.memory.buffer_occupation_ratio,
                        stat->node_stat.memory.msg_drop_number,
                        stat->node_stat.max_loop_time_ms,
                        stat->container_stat.max_retry);
            }
            break;
        case IO_STATE:
            // check size
            if (msg->header.size == sizeof(char))
            {
                // Size ok, now fill the struct from msg data
                //create the Json content
                if (msg->data[0])
                {
                    memcpy(data, "\"io_state\":true,", sizeof("\"io_state\":true,"));
                }
                else
                {
                    memcpy(data, "\"io_state\":false,", sizeof("\"io_state\":false,"));
                }
            }
            break;
        case EULER_3D:
        case COMPASS_3D:
        case GYRO_3D:
        case ACCEL_3D:
        case LINEAR_ACCEL:
        case GRAVITY_VECTOR:
            // check size
            if (msg->header.size == (3 * sizeof(float)))
            {
                // Size ok, now fill the struct from msg data
                float value[3];
                int32_t integerPart[3];
                uint32_t decimalPart[3];
                memcpy(value, msg->data, msg->header.size);
                char name[20] = {0};
                switch (msg->header.cmd)
                {
                    case LINEAR_ACCEL:
                        strcpy(name, "linear_accel");
                        break;
                    case GRAVITY_VECTOR:
                        strcpy(name, "gravity_vector");
                        break;
                    case COMPASS_3D:
                        strcpy(name, "compass");
                        break;
                    case GYRO_3D:
                        strcpy(name, "gyro");
                        break;
                    case ACCEL_3D:
                        strcpy(name, "accel");
                        break;
                    case EULER_3D:
                        strcpy(name, "euler");
                        break;
                }
                // Create the Json content
                // Convert floats
                Convert_SplitFloat(value[0], &integerPart[0], &decimalPart[0]);
                Convert_SplitFloat(value[1], &integerPart[1], &decimalPart[1]);
                Convert_SplitFloat(value[2], &integerPart[2], &decimalPart[2]);
                sprintf(data, "\"%s\":[%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 "],",
                        name,
                        integerPart[0], decimalPart[0],
                        integerPart[1], decimalPart[1],
                        integerPart[2], decimalPart[2]);
            }
            break;
        case QUATERNION:
            // check size
            if (msg->header.size == (4 * sizeof(float)))
            {
                // Size ok, now fill the struct from msg data
                float value[4];
                int32_t integerPart[4];
                uint32_t decimalPart[4];
                memcpy(value, msg->data, msg->header.size);
                //create the Json content
                // Convert floats
                Convert_SplitFloat(value[0], &integerPart[0], &decimalPart[0]);
                Convert_SplitFloat(value[1], &integerPart[1], &decimalPart[1]);
                Convert_SplitFloat(value[2], &integerPart[2], &decimalPart[2]);
                Convert_SplitFloat(value[3], &integerPart[3], &decimalPart[3]);
                sprintf(data, "\"quaternion\":[%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 "],",
                        integerPart[0], decimalPart[0],
                        integerPart[1], decimalPart[1],
                        integerPart[2], decimalPart[2],
                        integerPart[3], decimalPart[3]);
            }
            break;
        case ROT_MAT:
            // check size
            if (msg->header.size == (9 * sizeof(float)))
            {
                // Size ok, now fill the struct from msg data
                float value[9];
                int32_t integerPart[9];
                uint32_t decimalPart[9];
                memcpy(value, msg->data, msg->header.size);
                //create the Json content
                Convert_SplitFloat(value[0], &integerPart[0], &decimalPart[0]);
                Convert_SplitFloat(value[1], &integerPart[1], &decimalPart[1]);
                Convert_SplitFloat(value[2], &integerPart[2], &decimalPart[2]);
                Convert_SplitFloat(value[3], &integerPart[3], &decimalPart[3]);
                Convert_SplitFloat(value[4], &integerPart[4], &decimalPart[4]);
                Convert_SplitFloat(value[5], &integerPart[5], &decimalPart[5]);
                Convert_SplitFloat(value[6], &integerPart[6], &decimalPart[6]);
                Convert_SplitFloat(value[7], &integerPart[7], &decimalPart[7]);
                Convert_SplitFloat(value[8], &integerPart[8], &decimalPart[8]);
                sprintf(data, "\"rotational_matrix\":[%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 ",%" PRId32 ".%" PRIu32 "],",
                        integerPart[0], decimalPart[0],
                        integerPart[1], decimalPart[1],
                        integerPart[2], decimalPart[2],
                        integerPart[3], decimalPart[3],
                        integerPart[4], decimalPart[4],
                        integerPart[5], decimalPart[5],
                        integerPart[6], decimalPart[6],
                        integerPart[7], decimalPart[7],
                        integerPart[8], decimalPart[8]);
            }
            break;
        case HEADING:
            // check size
            if (msg->header.size == (sizeof(float)))
            {
                // Size ok, now fill the struct from msg data
                float value;
                int32_t integerPart;
                uint32_t decimalPart;
                memcpy(&value, msg->data, msg->header.size);
                //create the Json content

                Convert_SplitFloat(value, &integerPart, &decimalPart);
                sprintf(data, "\"heading\":%" PRId32 ".%" PRIu32 ",", integerPart, decimalPart);
            }
            break;
        case PEDOMETER:
            // check size
            if (msg->header.size == (2 * sizeof(unsigned long)))
            {
                // Size ok, now fill the struct from msg data
                unsigned long value[2];
                memcpy(value, msg->data, msg->header.size);
                //create the Json content
                sprintf(data, "\"pedometer\":%2ld,\"walk_time\":%2ld,", value[0], value[1]);
            }
            break;
        default:
            break;
    }
    data += strlen(data);
}
// This function end a Service into a Json structure and return the string size.
void Convert_EndServiceData(char *data)
{
    if (*data != '{')
    {
        // remove the last "," char
        *(--data) = '\0';
    }
    // End the container section
    memcpy(data, "},", sizeof("},"));
    data += sizeof("},") - 1;
}
// This function start a Json structure and return the string size.
void Convert_EndData(container_t *service, char *data, char *data_ptr)
{
    // remove the last "," char
    *(--data_ptr) = '\0';
    // End the Json message
    memcpy(data_ptr, "}}\n", sizeof("}}\n"));
    data_ptr += sizeof("}}\n") - 1;
    // Send the message to pipe
    PipeLink_Send(service, data, data_ptr - data);
}
// If there is no message receive for sometime we need to send void Json for synchronization.
void Convert_VoidData(container_t *service)
{
    char data[sizeof("{}\n")] = "{}\n";
    PipeLink_Send(service, data, sizeof("{}\n"));
}

/*******************************************************************************
 * Luos default information to Json convertion
 ******************************************************************************/
// This function generate a Json about assertion and send it.
void Convert_AssertToData(container_t *service, uint16_t source, luos_assert_t assertion)
{
    char assert_json[512];
    sprintf(assert_json, "{\"assert\":{\"node_id\":%d,\"file\":\"%s\",\"line\":%d}}\n", source, assertion.file, (unsigned int)assertion.line);
    // Send the message to pipe
    PipeLink_Send(service, assert_json, strlen(assert_json));
}
// This function generate a Json about excluded services and send it.
void Convert_ExcludedContainerData(container_t *service)
{
    char json[300];
    sprintf(json, "{\"dead_container\":\"%s\"", RoutingTB_AliasFromId(service->ll_container->dead_container_spotted));
    sprintf(json, "%s}\n", json);
    // Send the message to pipe
    PipeLink_Send(service, json, strlen(json));
}
// This function is directly called by Luos_utils in case of curent node assert. DO NOT RENAME IT
void node_assert(char *file, uint32_t line)
{
    // manage self crashing scenario
    char json[512];
    sprintf(json, "{\"assert\":{\"node_id\":1,\"file\":\"%s\",\"line\":%d}}\n", file, (unsigned int)line);
    // Send the message to pipe
    PipeLink_Send(0, json, strlen(json));
}

/*******************************************************************************
 * Luos routing table information to Json convertion
 ******************************************************************************/
void Convert_RoutingTableData(container_t *service)
{
    // Init the json string
    char json[GATE_BUFF_SIZE * 2];
    char *json_ptr = json;
    sprintf(json_ptr, "{\"routing_table\":[");
    json_ptr += strlen(json_ptr);
    // loop into containers.
    routing_table_t *routing_table = RoutingTB_Get();
    int last_entry                 = RoutingTB_GetLastEntry();
    int i                          = 0;
    //for (uint8_t i = 0; i < last_entry; i++) { //TODO manage all entries, not only containers.
    while (i < last_entry)
    {
        if (routing_table[i].mode == NODE)
        {
            sprintf(json_ptr, "{\"node_id\":%d", routing_table[i].node_id);
            json_ptr += strlen(json_ptr);
            if (routing_table[i].certified)
            {
                sprintf(json_ptr, ",\"certified\":true");
                json_ptr += strlen(json_ptr);
            }
            else
            {
                sprintf(json_ptr, ",\"certified\":false");
                json_ptr += strlen(json_ptr);
            }
            sprintf(json_ptr, ",\"port_table\":[");
            json_ptr += strlen(json_ptr);
            // Port loop
            for (int port = 0; port < 4; port++)
            {
                if (routing_table[i].port_table[port])
                {
                    sprintf(json_ptr, "%d,", routing_table[i].port_table[port]);
                    json_ptr += strlen(json_ptr);
                }
                else
                {
                    // remove the last "," char
                    *(--json_ptr) = '\0';
                    break;
                }
            }
            sprintf(json_ptr, "],\"containers\":[");
            json_ptr += strlen(json_ptr);
            i++;
            // Containers loop
            while (i < last_entry)
            {
                if (routing_table[i].mode == CONTAINER)
                {
                    // Create container description
                    sprintf(json_ptr, "{\"type\":\"%s\",\"id\":%d,\"alias\":\"%s\"},", RoutingTB_StringFromType(routing_table[i].type), routing_table[i].id, routing_table[i].alias);
                    json_ptr += strlen(json_ptr);
                    i++;
                }
                else
                    break;
            }
            // remove the last "," char
            *(--json_ptr) = '\0';
            sprintf(json_ptr, "]},");
            json_ptr += strlen(json_ptr);
        }
        else
        {
            i++;
        }
    }
    // remove the last "," char
    *(--json_ptr) = '\0';
    // End the Json message
    sprintf(json_ptr, "]}\n");
    // Send the message to pipe
    PipeLink_Send(service, json, strlen(json));
}