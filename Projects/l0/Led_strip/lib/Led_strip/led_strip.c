/******************************************************************************
 * @file led strip
 * @brief driver example a simple led strip
 * @author Luos
 * @version 0.0.0
 ******************************************************************************/
#include "led_strip.h"
#include "main.h"
#include "tim.h"
#include "luos.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define MAX_LED_NUMBER   375
#define OVERHEAD         (9 * 24) // Number of data to add to create a reset between frames
#define DECOMP_BUFF_SIZE (MAX_LED_NUMBER * 24 + OVERHEAD)

/*******************************************************************************
 * Variables
 ******************************************************************************/
volatile color_t matrix[MAX_LED_NUMBER];
volatile char buf[DECOMP_BUFF_SIZE] = {0};
volatile int imgsize                = MAX_LED_NUMBER;

/*******************************************************************************
 * Function
 ******************************************************************************/
static void LedStrip_MsgHandler(service_t *service, msg_t *msg);
static void convert_color(color_t color, int led_nb);
static void image_size(int size);

/******************************************************************************
 * @brief init must be call in project init
 * @param None
 * @return None
 ******************************************************************************/
void LedStrip_Init(void)
{
    revision_t revision = {.unmap = REV};
    Luos_CreateService(LedStrip_MsgHandler, COLOR_TYPE, "led_strip", revision);
    TIM2->CCR1 = 0;
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop_DMA(&htim2, TIM_CHANNEL_1);
    memset((void *)buf, 0, DECOMP_BUFF_SIZE);
    memset((void *)matrix, 0, MAX_LED_NUMBER * 3);
    HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_1, (uint32_t *)buf, DECOMP_BUFF_SIZE);
}
/******************************************************************************
 * @brief loop must be call in project loop
 * @param None
 * @return None
 ******************************************************************************/
void LedStrip_Loop(void)
{
    // Convert matrix into stream data
    for (int i = 0; i < MAX_LED_NUMBER; i++)
    {
        convert_color(matrix[i], i);
    }
}
/******************************************************************************
 * @brief Msg Handler call back when a msg receive for this service
 * @param Service destination
 * @param Msg receive
 * @return None
 ******************************************************************************/
static void LedStrip_MsgHandler(service_t *service, msg_t *msg)
{
    if (msg->header.cmd == COLOR)
    {
        // change led target color
        if (msg->header.size == 3)
        {
            // there is only one color copy it in the entire matrix
            for (int i = 0; i < imgsize; i++)
            {
                memcpy((void *)matrix + i, msg->data, sizeof(color_t));
            }
        }
        else
        {
            // image management
            Luos_ReceiveData(service, msg, (void *)matrix);
        }
        return;
    }
    if (msg->header.cmd == PARAMETERS)
    {
        // set the led strip size
        short size;
        memcpy(&size, msg->data, sizeof(short));
        image_size(size);
        return;
    }
}

static void image_size(int size)
{
    memset((void *)matrix + size, 0, (MAX_LED_NUMBER - size) * 3);
    imgsize = size;
}

static void convert_color(color_t color, int led_nb)
{ // It could be GRB
    char remap[3] = {color.g, color.r, color.b};
    for (int y = 0; y < 3; y++)
    {
        for (int i = 0; i < 8; i++)
        {
            if (remap[y] & (1 << (7 - i)))
            {
                buf[(led_nb * 24) + ((y * 8) + i)] = 38;
            }
            else
            {
                buf[(led_nb * 24) + ((y * 8) + i)] = 19;
            }
        }
    }
}
