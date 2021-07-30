/******************************************************************************
 * @file controller_motor
 * @brief driver example a simple controller motor
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "controller_motor.h"

#include "main.h"
#include "stdbool.h"
#include "analog.h"
#include "tim.h"
#include "math.h"
#include <float.h>
#include "template_servo_motor.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define ASSERV_PERIOD        1
#define SPEED_PERIOD         50
#define SPEED_NB_INTEGRATION SPEED_PERIOD / ASSERV_PERIOD
#define SAMPLING_PERIOD_MS   10.0
#define BUFFER_SIZE          1000

// Pin configuration
#define FB_Pin       GPIO_PIN_0
#define FB_GPIO_Port GPIOB

/*******************************************************************************
 * Variables
 ******************************************************************************/

template_servo_motor_t servo_motor_template;
profile_servo_motor_t *servo_motor = &servo_motor_template.profile;

float errSpeedSum            = 0.0;
float motion_target_position = 0.0;

// Position Asserv things
volatile float errAngleSum  = 0.0;
volatile float lastErrAngle = 0.0;

// Speed Asserv things
volatile float lastErrSpeed = 0.0;

// Trajectory management (can be position or speed)
volatile float trajectory_buf[BUFFER_SIZE];
volatile angular_position_t last_position = 0.0;

// measurement management (can be position or speed)
volatile float measurement_buf[BUFFER_SIZE];

// Speed calculation values
char speed_bootstrap = 0;
/*******************************************************************************
 * Function
 ******************************************************************************/
static void ControllerMotor_MsgHandler(service_t *service, msg_t *msg);
static void set_ratio(ratio_t ratio);
static void enable_motor(char state);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void ControllerMotor_Init(void)
{
    revision_t revision = {.major = 1, .minor = 0, .build = 0};
    // ******************* Analog measurement *******************
    // interesting tutorial about ADC : https://visualgdb.com/tutorials/arm/stm32/adc/
    ADC_ChannelConfTypeDef sConfig   = {0};
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    // Enable  ADC Gpio clocks
    //__HAL_RCC_GPIOA_CLK_ENABLE(); => already enabled previously
    /**ADC GPIO Configuration
    */
    GPIO_InitStruct.Pin  = FB_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(FB_GPIO_Port, &GPIO_InitStruct);
    // Enable  ADC clocks
    __HAL_RCC_ADC1_CLK_ENABLE();
    // Setup Adc to loop on DMA continuously
    ControllerMotor_adc.Instance                   = ADC1;
    ControllerMotor_adc.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;
    ControllerMotor_adc.Init.Resolution            = ADC_RESOLUTION_12B;
    ControllerMotor_adc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    ControllerMotor_adc.Init.ScanConvMode          = ADC_SCAN_ENABLE;
    ControllerMotor_adc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    ControllerMotor_adc.Init.LowPowerAutoWait      = DISABLE;
    ControllerMotor_adc.Init.LowPowerAutoPowerOff  = DISABLE;
    ControllerMotor_adc.Init.ContinuousConvMode    = ENABLE;
    ControllerMotor_adc.Init.DiscontinuousConvMode = DISABLE;
    ControllerMotor_adc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    ControllerMotor_adc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    ControllerMotor_adc.Init.DMAContinuousRequests = ENABLE;
    ControllerMotor_adc.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    if (HAL_ADC_Init(&ControllerMotor_adc) != HAL_OK)
    {
        Error_Handler();
    }
    /** Configure voltage input channel. */
    sConfig.Channel      = ADC_CHANNEL_8;
    sConfig.Rank         = ADC_RANK_CHANNEL_NUMBER;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_ConfigChannel(&ControllerMotor_adc, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }
    // Enable DMA1 clock
    __HAL_RCC_DMA1_CLK_ENABLE();
    /* ADC1 DMA Init */
    /* ADC Init */
    ControllerMotor_dma_adc.Instance                 = DMA1_Channel1;
    ControllerMotor_dma_adc.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    ControllerMotor_dma_adc.Init.PeriphInc           = DMA_PINC_DISABLE;
    ControllerMotor_dma_adc.Init.MemInc              = DMA_MINC_ENABLE;
    ControllerMotor_dma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    ControllerMotor_dma_adc.Init.MemDataAlignment    = DMA_MDATAALIGN_WORD;
    ControllerMotor_dma_adc.Init.Mode                = DMA_CIRCULAR;
    ControllerMotor_dma_adc.Init.Priority            = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&ControllerMotor_dma_adc) != HAL_OK)
    {
        Error_Handler();
    }
    __HAL_LINKDMA(&ControllerMotor_adc, DMA_Handle, ControllerMotor_dma_adc);
    // disable DMA Irq
    HAL_NVIC_DisableIRQ(DMA1_Channel1_IRQn);
    // Start infinite ADC measurement
    HAL_ADC_Start_DMA(&ControllerMotor_adc, (uint32_t *)analog_input.unmap, sizeof(analog_input_t) / sizeof(uint32_t));
    // ************** Pwm settings *****************
    servo_motor->sampling_period = TimeOD_TimeFrom_ms(SAMPLING_PERIOD_MS);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);
    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_1 | TIM_CHANNEL_2);

    // ************** Default configuration settings *****************
    // motor mode by default
    enable_motor(0);
    servo_motor->mode.mode_compliant        = 1;
    servo_motor->mode.current               = 0;
    servo_motor->mode.mode_power            = 1;
    servo_motor->mode.mode_angular_position = 0;
    servo_motor->mode.mode_angular_speed    = 0;
    servo_motor->mode.mode_linear_position  = 0;
    servo_motor->mode.mode_linear_speed     = 0;
    servo_motor->mode.angular_position      = 1;
    servo_motor->mode.angular_speed         = 0;
    servo_motor->mode.linear_position       = 0;
    servo_motor->mode.linear_speed          = 0;

    // default motor configuration
    servo_motor->motor_reduction = 131;
    servo_motor->resolution      = 16;
    servo_motor->wheel_diameter  = 0.100f;

    // default motor limits
    servo_motor->motor.limit_ratio            = 100.0;
    servo_motor->limit_angular_position[MINI] = -FLT_MAX;
    servo_motor->limit_angular_position[MAXI] = FLT_MAX;
    servo_motor->motor.limit_current          = 6.0;

    // Position PID default values
    servo_motor->position_pid.p = 4.0;
    servo_motor->position_pid.i = 0.02;
    servo_motor->position_pid.d = 100.0;

    // Speed PID default values
    servo_motor->speed_pid.p = 0.1;
    servo_motor->speed_pid.i = 0.1;
    servo_motor->speed_pid.d = 0.0;

    // Control mode default values
    servo_motor->control.unmap = 0; // PLAY and no REC

    // Init streaming channels
    servo_motor->trajectory  = Stream_CreateStreamingChannel((float *)trajectory_buf, BUFFER_SIZE, sizeof(float));
    servo_motor->measurement = Stream_CreateStreamingChannel((float *)measurement_buf, BUFFER_SIZE, sizeof(float));

    // ************** Service creation *****************
    TemplateServoMotor_CreateService(ControllerMotor_MsgHandler, &servo_motor_template, "servo_motor", revision);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void ControllerMotor_Loop(void)
{
    // Time management
    static uint32_t last_asserv_systick = 0;
    uint32_t timestamp                  = HAL_GetTick();
    uint32_t deltatime                  = timestamp - last_asserv_systick;

    // Speed measurement
    static angular_position_t last_angular_positions[SPEED_NB_INTEGRATION];

    // ************* Values computation *************
    // angular_posistion => degree
    int32_t encoder_count = (int16_t)TIM2->CNT;
    TIM2->CNT             = 0;
    servo_motor->angular_position += AngularOD_PositionFrom_deg(((double)encoder_count / (double)(servo_motor->motor_reduction * servo_motor->resolution * 4)) * 360.0);
    // linear_distance => m
    servo_motor->linear_position = LinearOD_PositionFrom_m((servo_motor->angular_position / 360.0) * M_PI * servo_motor->wheel_diameter);
    // current => A
    servo_motor->motor.current = ElectricOD_CurrentFrom_A(((((float)analog_input.current) * 3.3f) / 4096.0f) / 0.525f);

    if (deltatime >= ASSERV_PERIOD)
    {
        last_asserv_systick = timestamp;
        // angular_speed => degree/seconds
        // add the position value into unfiltered speed measurement
        for (int nbr = 0; nbr < (SPEED_NB_INTEGRATION - 1); nbr++)
        {
            // Check if this is the first measurement. If it is init the table.
            if (!speed_bootstrap)
            {
                last_angular_positions[nbr] = servo_motor->angular_position;
            }
            else
            {
                last_angular_positions[nbr] = last_angular_positions[nbr + 1];
            }
        }
        speed_bootstrap                                  = 1;
        last_angular_positions[SPEED_NB_INTEGRATION - 1] = servo_motor->angular_position;
        servo_motor->angular_speed                       = AngularOD_SpeedFrom_deg_s((last_angular_positions[SPEED_NB_INTEGRATION - 1] - last_angular_positions[0]) * 1000.0 / SPEED_PERIOD);
        // linear_speed => m/seconds
        servo_motor->linear_speed = LinearOD_Speedfrom_m_s((servo_motor->angular_speed / 360.0) * M_PI * servo_motor->wheel_diameter);
        // ************* Limit clamping *************
        if (motion_target_position < servo_motor->limit_angular_position[MINI])
        {
            motion_target_position = servo_motor->limit_angular_position[MINI];
        }
        if (motion_target_position > servo_motor->limit_angular_position[MAXI])
        {
            motion_target_position = servo_motor->limit_angular_position[MAXI];
        }
        float currentfactor         = 1.0f;
        currentfactor               = servo_motor->motor.limit_current / (servo_motor->motor.current * 2);
        static float surpCurrentSum = 0.0;
        float surpCurrent           = servo_motor->motor.current - servo_motor->motor.limit_current;
        surpCurrentSum += surpCurrent;
        // If surpCurrentSum > 0 do a real coef
        if (surpCurrentSum > 0.0)
        {
            currentfactor = servo_motor->motor.limit_current / (servo_motor->motor.limit_current + (surpCurrentSum / 1.5));
        }
        else
        {
            surpCurrentSum = 0.0;
            currentfactor  = 1.0f;
        }
        if (servo_motor->mode.mode_compliant)
        {
            //Motor is compliant, only manage motor limits
            if (servo_motor->angular_position < servo_motor->limit_angular_position[MINI])
            {
                //re-enable motor to avoid bypassing motors limits
                enable_motor(1);
                set_ratio(100.0 * (servo_motor->limit_angular_position[MINI] - servo_motor->angular_position));
            }
            else if (servo_motor->angular_position > servo_motor->limit_angular_position[MAXI])
            {
                enable_motor(1);
                set_ratio(-100.0 * (servo_motor->angular_position - servo_motor->limit_angular_position[MAXI]));
            }
            else
            {
                enable_motor(0);
            }
        }
        else if (servo_motor->mode.mode_power)
        {
            set_ratio(servo_motor->motor.power * currentfactor);
        }
        else
        {
            // ************* position asserv *************
            // Target Position is managed by the motion planning interrupt (systick interrupt)
            float errAngle   = 0.0;
            float dErrAngle  = 0.0;
            float anglePower = 0.0;
            if (servo_motor->mode.mode_angular_position || servo_motor->mode.mode_linear_position)
            {
                errAngle  = motion_target_position - servo_motor->angular_position;
                dErrAngle = (errAngle - lastErrAngle) / deltatime;
                errAngleSum += (errAngle * (float)deltatime);
                // Integral clamping
                if (errAngleSum < -100.0)
                    errAngleSum = -100.0;
                if (errAngleSum > 100.0)
                    errAngleSum = 100;
                anglePower   = (errAngle * servo_motor->position_pid.p) + (errAngleSum * servo_motor->position_pid.i) + (dErrAngle * servo_motor->position_pid.d); // raw PID command
                lastErrAngle = errAngle;
            }
            // ************* speed asserv *************
            float errSpeed   = 0.0;
            float dErrSpeed  = 0.0;
            float speedPower = 0.0;
            if (servo_motor->mode.mode_angular_speed || servo_motor->mode.mode_linear_speed)
            {
                errSpeed  = servo_motor->target_angular_speed - servo_motor->angular_speed;
                dErrSpeed = (errSpeed - lastErrSpeed) / deltatime;
                errSpeedSum += (errSpeed * (float)deltatime);
                if (errSpeedSum < -100.0)
                    errSpeedSum = -100.0;
                if (errSpeedSum > 100.0)
                    errSpeedSum = 100;
                speedPower   = ((errSpeed * servo_motor->speed_pid.p) + (errSpeedSum * servo_motor->speed_pid.i) + (dErrSpeed * servo_motor->speed_pid.d)); // raw PID command
                lastErrSpeed = errSpeed;
            }
            // ************* command merge *************
            if (!(servo_motor->mode.mode_angular_position || servo_motor->mode.mode_linear_position) && (servo_motor->mode.mode_angular_speed || servo_motor->mode.mode_linear_speed))
            {
                // Speed control only
                set_ratio(speedPower * currentfactor);
            }
            else
            {
                // we use position control by default
                set_ratio(anglePower * currentfactor);
            }
        }
    }
}
/******************************************************************************
 * @brief Msg manager call by luos when service created a msg receive
 * @param Service send msg
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void ControllerMotor_MsgHandler(service_t *service, msg_t *msg)
{
    if (msg->header.cmd == GET_CMD)
    {
        return;
    }
    if (msg->header.cmd == PARAMETERS)
    {
        enable_motor(servo_motor->mode.mode_compliant == 0);
        if (servo_motor->mode.mode_compliant == 0)
        {
            __disable_irq();
            last_position = servo_motor->angular_position;
            errAngleSum   = 0.0;
            lastErrAngle  = 0.0;
            __enable_irq();
        }
        return;
    }
    if (msg->header.cmd == REINIT)
    {
        // reinit asserv calculation
        errAngleSum     = 0.0;
        lastErrAngle    = 0.0;
        last_position   = 0.0;
        speed_bootstrap = 0;
        return;
    }
    if ((msg->header.cmd == ANGULAR_POSITION) || (msg->header.cmd == LINEAR_POSITION))
    {
        if ((servo_motor->mode.mode_angular_position | servo_motor->mode.mode_angular_position) && (msg->header.size == sizeof(angular_position_t)))
        {
            // set the motor target angular position
            __disable_irq();
            last_position = servo_motor->angular_position;
            __enable_irq();
        }
        return;
    }
    if ((msg->header.cmd == ANGULAR_SPEED) || (msg->header.cmd == LINEAR_SPEED))
    {
        // set the motor target angular position
        if ((servo_motor->mode.mode_angular_speed) | (servo_motor->mode.mode_linear_speed))
        {
            // reset the integral factor for speed
            errSpeedSum = 0.0;
        }
        return;
    }
}

void HAL_SYSTICK_Motor_Callback(void)
{
    // ************* motion planning *************
    // ****** recorder management *********
    static uint32_t last_rec_systick = 0;
    if (servo_motor->control.rec && ((HAL_GetTick() - last_rec_systick) >= TimeOD_TimeTo_ms(servo_motor->sampling_period)))
    {
        // We have to save a sample of current position
        Stream_PutSample(&servo_motor->measurement, (angular_position_t *)&servo_motor->angular_position, 1);
        last_rec_systick = HAL_GetTick();
    }
    // ****** trajectory management *********
    static uint32_t last_systick = 0;
    if (servo_motor->control.flux == STOP)
    {
        Stream_ResetStreamingChannel(&servo_motor->trajectory);
    }
    if ((Stream_GetAvailableSampleNB(&servo_motor->trajectory) > 0) && ((HAL_GetTick() - last_systick) >= TimeOD_TimeTo_ms(servo_motor->sampling_period)) && (servo_motor->control.flux == PLAY))
    {
        if (servo_motor->mode.mode_linear_position == 1)
        {
            linear_position_t linear_position_tmp;
            Stream_GetSample(&servo_motor->trajectory, &linear_position_tmp, 1);
            servo_motor->target_angular_position = (linear_position_tmp * 360.0) / (3.141592653589793 * servo_motor->wheel_diameter);
        }
        else
        {
            Stream_GetSample(&servo_motor->trajectory, (angular_position_t *)&servo_motor->target_angular_position, 1);
        }
        last_systick = HAL_GetTick();
    }
    // ****** Linear interpolation *********
    if ((servo_motor->mode.mode_angular_position || servo_motor->mode.mode_linear_position)
        && (servo_motor->mode.mode_angular_speed || servo_motor->mode.mode_linear_speed))
    {

        // speed control and position control are enabled
        // we need to move target position following target speed
        float increment = (fabs(servo_motor->target_angular_speed) / 1000.0);
        if (fabs(servo_motor->target_angular_position - last_position) <= increment)
        {
            // target_position is the final target position
            motion_target_position = servo_motor->target_angular_position;
        }
        else if ((servo_motor->target_angular_position - servo_motor->angular_position) < 0.0)
        {
            motion_target_position = last_position - increment;
        }
        else
        {
            motion_target_position = last_position + increment;
        }
    }
    else
    {
        // target_position is the final target position
        motion_target_position = servo_motor->target_angular_position;
    }
    last_position = motion_target_position;
}

static void set_ratio(ratio_t ratio)
{
    // limit power value
    if (ratio < -servo_motor->motor.limit_ratio)
        ratio = -servo_motor->motor.limit_ratio;
    if (ratio > servo_motor->motor.limit_ratio)
        ratio = servo_motor->motor.limit_ratio;
    // transform power ratio to timer value
    uint16_t pulse;
    if (RatioOD_RatioToPercent(ratio) > 0.0)
    {
        pulse      = (uint16_t)(RatioOD_RatioToPercent(ratio) * 24.0);
        TIM3->CCR1 = pulse;
        TIM3->CCR2 = 0;
    }
    else
    {
        pulse      = (uint16_t)(-RatioOD_RatioToPercent(ratio) * 24.0);
        TIM3->CCR1 = 0;
        TIM3->CCR2 = pulse;
    }
}

static void enable_motor(char state)
{
    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, state);
}
