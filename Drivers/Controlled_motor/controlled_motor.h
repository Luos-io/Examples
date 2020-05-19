#ifndef CONTROLLED_MOTOR_H
#define CONTROLLED_MOTOR_H

#include "luos.h"

// Motor
typedef struct __attribute__((__packed__))
{
    union {
        struct __attribute__((__packed__))
        {
            // target modes
            uint16_t mode_compliant : 1;
            uint16_t mode_ratio : 1;
            uint16_t mode_torque : 1;
            uint16_t mode_angular_speed : 1;
            uint16_t mode_angular_position : 1;
            uint16_t mode_linear_speed : 1;
            uint16_t mode_linear_position : 1;

            // report modes
            uint16_t angular_position : 1;
            uint16_t angular_speed : 1;
            uint16_t linear_position : 1;
            uint16_t linear_speed : 1;
            uint16_t current : 1;
            uint16_t temperature : 1;
        };
        uint8_t unmap[2];
    };
} motor_mode_t;

typedef struct __attribute__((__packed__))
{
    // targets
    motor_mode_t mode;
    angular_position_t target_angular_position;
    angular_speed_t target_angular_speed;
    ratio_t target_ratio;

    // limits
    angular_position_t limit_angular_position[2];
    ratio_t limit_ratio;
    current_t limit_current;

    // measures
    angular_position_t angular_position;
    angular_speed_t angular_speed;
    linear_position_t linear_position;
    linear_speed_t linear_speed;
    current_t current;
    temperature_t temperature;

    //configs
    float motor_reduction;
    float resolution;
    linear_position_t wheel_diameter;
} motor_config_t;

void controlled_motor_init(void);
void controlled_motor_loop(void);

#endif /* CONTROLLED_MOTOR_H */
