/******************************************************************************
 * @file motor
 * @brief FOC motor driver
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
using namespace std;
#include <Arduino.h>
#include "motor.h"
#include <SimpleFOC.h>
#include "SPI.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*******************************************************************************
 * Variables
 ******************************************************************************/
SPIClass SPI_FOC(&sercom1, 12, 13, 11, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_3);
MagneticSensorSPI sensor = MagneticSensorSPI(AS5047_SPI, 10);

BLDCMotor motor       = BLDCMotor(14);
BLDCDriver3PWM driver = BLDCDriver3PWM(9, 5, 6, 8);

float target_angle = 150;
/*******************************************************************************
 * Function
 ******************************************************************************/
static void Motor_MsgHandler(service_t *service, msg_t *msg);
/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Motor_Init(void)
{
    // initialize sensor
    sensor.init(&SPI_FOC);

    // initialize driver
    driver.voltage_power_supply = 12;
    driver.init();

    // initialize motor
    motor.linkSensor(&sensor);
    motor.linkDriver(&driver);
    // set motion control loop to be used
    motor.controller = MotionControlType::angle;

    // velocity PI controller parameters
    motor.PID_velocity.P = 0.2f;
    motor.PID_velocity.I = 20;
    motor.PID_velocity.D = 0;
    // maximal voltage to be set to the motor
    motor.voltage_limit = 6;

    // velocity low pass filtering time constant
    // the lower the less filtered
    motor.LPF_velocity.Tf = 0.01f;

    // angle P controller
    motor.P_angle.P = 20;
    // maximal velocity of the position control
    motor.velocity_limit = 20;

    // initialize motor
    motor.init();
    // // align sensor and start FOC
    motor.initFOC();

    // initialize service
    revision_t revision = {1, 0, 0};
    Luos_CreateService(Motor_MsgHandler, MOTOR_TYPE, "FOC_motor", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Motor_Loop(void)
{
    // main FOC algorithm function
    motor.loopFOC();

    // Motion control function
    // velocity, position or voltage (defined in motor.controller)
    // this function can be run at much lower frequency than loopFOC() function
    // You can also use motor.move() and set the motor.target in the code
    motor.move(target_angle);
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this service
 * @param Service destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Motor_MsgHandler(service_t *service, msg_t *msg)
{
}
