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

#include "wiring_private.h"
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

#define GEAR_RATE 10

// angular prosition command
float angle_command = 0.0f;
float angle_read    = 0.0f;
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

    pinPeripheral(11, PIO_SERCOM);
    pinPeripheral(12, PIO_SERCOM);
    pinPeripheral(13, PIO_SERCOM);

    // initialize driver
    driver.voltage_power_supply = 12;
    driver.init();

    // initialize motor
    motor.linkSensor(&sensor);
    motor.linkDriver(&driver);

    // choose FOC modulation (optional)
    motor.foc_modulation = FOCModulationType::SpaceVectorPWM;

    // set motion control loop to be used
    motor.controller = MotionControlType::angle;
    // motor.controller = MotionControlType::velocity;

    // contoller configuration
    // default parameters in defaults.h

    // velocity PI controller parameters
    motor.PID_velocity.P = 0.2f;
    motor.PID_velocity.I = 20;
    motor.PID_velocity.D = 0;
    // maximal voltage to be set to the motor
    motor.voltage_limit = 2.0;

    // velocity low pass filtering time constant
    // the lower the less filtered
    motor.LPF_velocity.Tf = 0.01f;

    // angle P controller
    motor.P_angle.P = 20;
    // maximal velocity of the position control
    motor.velocity_limit = 20.0;

    // init motor hardware
    motor.init();
    // align sensor and start FOC
    motor.initFOC();

    // initialize service
    revision_t revision = {1, 0, 0};
    Luos_CreateService(Motor_MsgHandler, SERVO_MOTOR_TYPE, "FOC_motor", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Motor_Loop(void)
{
    // main FOC algorithm function
    // the faster you run this function the better
    // Arduino UNO loop  ~1kHz
    // Bluepill loop ~10kHz
    motor.loopFOC();

    // Motion control function
    // velocity, position or voltage (defined in motor.controller)
    // this function can be run at much lower frequency than loopFOC() function
    // You can also use motor.move() and set the motor.target in the code
    motor.move(AngularOD_PositionTo_rad(angle_command) * GEAR_RATE);

    // update sensor position
    // don't know why there is a *5.7 coef to get the right value
    angle_read = sensor.getAngle() * 5.724417123;
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this service
 * @param Service destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Motor_MsgHandler(service_t *service, msg_t *msg)
{
    if ((msg->header.cmd == ANGULAR_POSITION))
    {
        AngularOD_PositionFromMsg((angular_position_t *)&angle_command, msg);
    }

    if ((msg->header.cmd == GET_CMD))
    {
        // Report management
        msg_t pub_msg;
        pub_msg.header.target_mode = msg->header.target_mode;
        pub_msg.header.target      = msg->header.source;

        AngularOD_PositionToMsg((angular_position_t *)&angle_read, &pub_msg);
        Luos_SendMsg(service, &pub_msg);
    }
}
