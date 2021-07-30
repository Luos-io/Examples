/******************************************************************************
 * @file distance
 * @brief driver example a simple distance
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "main.h"
#include "distance.h"
#include "vl53l0x_api.h"
#include "vl53l0x_platform.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ADDRESS_DEFAULT 0b0101001

/*******************************************************************************
 * Variables
 ******************************************************************************/
VL53L0X_RangingMeasurementData_t ranging_data;
uint8_t new_data_ready = 0;

VL53L0X_Dev_t dev;
/*******************************************************************************
 * Function
 ******************************************************************************/
static void Distance_MsgHandler(container_t *container, msg_t *msg);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void Distance_Init(void)
{
    revision_t revision = {.unmap = REV};
    //reset sensor
    HAL_GPIO_WritePin(SHUTDOWN_GPIO_Port, SHUTDOWN_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(SHUTDOWN_GPIO_Port, SHUTDOWN_Pin, GPIO_PIN_SET);
    HAL_Delay(5);

    dev.addr = 0x52;

    VL53L0X_DataInit(&dev);
    VL53L0X_StaticInit(&dev);

    uint32_t ref_spad_count;
    uint8_t is_aperture_spads;
    VL53L0X_PerformRefSpadManagement(&dev, &ref_spad_count, &is_aperture_spads);

    uint8_t vhv_settings;
    uint8_t phase_cal;
    VL53L0X_PerformRefCalibration(&dev, &vhv_settings, &phase_cal);

    //VL53L0X_PerformOffsetCalibration(&dev,distmm, offset_um);

    VL53L0X_SetDeviceMode(&dev, VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
    VL53L0X_StartMeasurement(&dev);
    Luos_CreateContainer(Distance_MsgHandler, DISTANCE_TYPE, "distance_mod", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void Distance_Loop(void)
{
    uint8_t data_ready = 0;
    VL53L0X_GetMeasurementDataReady(&dev, &data_ready);
    if (data_ready)
    {
        VL53L0X_GetRangingMeasurementData(&dev, &ranging_data);
        VL53L0X_ClearInterruptMask(&dev, VL53L0X_REG_SYSTEM_INTERRUPT_GPIO_NEW_SAMPLE_READY);
        new_data_ready++;
    }
}
/******************************************************************************
 * @brief Msg handler call back when a msg receive for this container
 * @param Container destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void Distance_MsgHandler(container_t *container, msg_t *msg)
{
    if ((msg->header.cmd == GET_CMD) & (new_data_ready))
    {
        msg_t pub_msg;

        linear_position_t dist = -0.001;
        if (ranging_data.RangeStatus == 0)
        {
            // dist measurement ok
            dist = LinearOD_PositionFrom_mm((float)ranging_data.RangeMilliMeter);
        }
        // fill the message infos
        pub_msg.header.target_mode = ID;
        pub_msg.header.target      = msg->header.source;
        LinearOD_PositionToMsg(&dist, &pub_msg);
        new_data_ready = 0;
        Luos_SendMsg(container, &pub_msg);
    }
}
