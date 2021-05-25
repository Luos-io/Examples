/******************************************************************************
 * @file convert
 * @brief Functions allowing to manage data convertion to a specific format
 * @author Luos
 ******************************************************************************/
#ifndef CONVERT_H_
#define CONVERT_H_

#include "luos.h"

/*
 * Pid
 */
typedef struct __attribute__((__packed__))
{
    union
    {
        struct __attribute__((__packed__))
        {
            float p;
            float i;
            float d;
        };
        unsigned char unmap[3 * sizeof(float)];
    };
} asserv_pid_t;

/*
 * Servo
 */
typedef struct
{
    union
    {
        struct __attribute__((__packed__))
        {
            angular_position_t max_angle;
            float min_pulse_time;
            float max_pulse_time;
        };
        unsigned char unmap[3 * sizeof(float)];
    };
} servo_parameters_t;

// Luos data to Luos messages convertion
void Convert_DataToLuos(container_t *service, char *data);

// Luos service information to Data convertion
void Convert_StartData(char *data);
void Convert_StartServiceData(char *data, char *alias);
void Convert_MsgToData(msg_t *msg, char *data);
void Convert_EndServiceData(char *data);
void Convert_EndData(container_t *service, char *data, char *data_ptr);
void Convert_VoidData(container_t *service);

// Luos default information to Data convertion
void Convert_AssertToData(container_t *service, uint16_t source, luos_assert_t assertion);
void Convert_ExcludedContainerData(container_t *service);

// Luos routing table information to Json convertion
void Convert_RoutingTableData(container_t *service);

#endif /* CONVERT_H_ */
