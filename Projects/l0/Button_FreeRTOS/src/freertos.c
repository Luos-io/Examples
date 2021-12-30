/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * File Name          : freertos.c
 * Description        : Code for freertos applications
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "luos.h"
#include "button.h"
#include "led.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
    .name       = "defaultTask",
    .stack_size = 128 * 4,
    .priority   = (osPriority_t)osPriorityNormal,
};
/* Definitions for IdleOS */
osThreadId_t IdleOSHandle;
const osThreadAttr_t IdleOS_attributes = {
    .name       = "IdleOS",
    .stack_size = 128 * 4,
    .priority   = (osPriority_t)osPriorityNormal,
};
/* Definitions for LuosTask */
osThreadId_t LuosTaskHandle;
const osThreadAttr_t LuosTask_attributes = {
    .name       = "LuosTask",
    .stack_size = 1024 * 4,
    .priority   = (osPriority_t)osPriorityHigh,
};

osThreadId_t ButtonTaskHandle;
const osThreadAttr_t ButtonTask_attributes = {
    .name       = "Button",
    .stack_size = 128 * 4,
    .priority   = (osPriority_t)osPriorityAboveNormal,
};

osThreadId_t LedTaskHandle;
const osThreadAttr_t LedTask_attributes = {
    .name       = "Led",
    .stack_size = 128 * 4,
    .priority   = (osPriority_t)osPriorityAboveNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartIdleOS(void *argument);
void StartLuosTask(void *argument);
void StartButtonTask(void *argument);
void StartLedTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
 * @brief  FreeRTOS initialization
 * @param  None
 * @retval None
 */
void MX_FREERTOS_Init(void)
{
    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* USER CODE BEGIN RTOS_MUTEX */
    /* add mutexes, ... */
    /* USER CODE END RTOS_MUTEX */

    /* USER CODE BEGIN RTOS_SEMAPHORES */
    /* add semaphores, ... */
    /* USER CODE END RTOS_SEMAPHORES */

    /* USER CODE BEGIN RTOS_TIMERS */
    /* start timers, add new ones, ... */
    /* USER CODE END RTOS_TIMERS */

    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    /* USER CODE END RTOS_QUEUES */

    /* Create the thread(s) */
    /* creation of defaultTask */
    defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

    /* creation of IdleOS */
    IdleOSHandle = osThreadNew(StartIdleOS, NULL, &IdleOS_attributes);

    /* creation of LuosTask */
    Luos_Init();
    Button_Init();
    Led_Init();

    LuosTaskHandle   = osThreadNew(StartLuosTask, NULL, &LuosTask_attributes);
    ButtonTaskHandle = osThreadNew(StartButtonTask, NULL, &ButtonTask_attributes);
    LedTaskHandle    = osThreadNew(StartLedTask, NULL, &LedTask_attributes);

    /* USER CODE BEGIN RTOS_THREADS */
    /* add threads, ... */
    /* USER CODE END RTOS_THREADS */

    /* USER CODE BEGIN RTOS_EVENTS */
    /* add events, ... */
    /* USER CODE END RTOS_EVENTS */
}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
 * @brief  Function implementing the defaultTask thread.
 * @param  argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
    /* USER CODE BEGIN StartDefaultTask */
    /* Infinite loop */
    while (1)
    {
        osDelay(1);
    }
    /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartIdleOS */
/**
 * @brief Function implementing the IdleOS thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartIdleOS */
void StartIdleOS(void *argument)
{
    /* USER CODE BEGIN StartIdleOS */
    /* Infinite loop */
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        osDelay(500);
    }
    /* USER CODE END StartIdleOS */
}

/* USER CODE BEGIN Header_StartLuosTask */
/**
 * @brief Function implementing the LuosTask thread.
 * @param argument: Not used
 * @retval None
 */
/* USER CODE END Header_StartLuosTask */
void StartLuosTask(void *argument)
{
    /* USER CODE BEGIN StartLuosTask */
    /* Infinite loop */
    while (1)
    {
        Luos_Loop();
    }
    /* USER CODE END StartLuosTask */
}

void StartButtonTask(void *argument)
{
    /* USER CODE BEGIN StartButtonTask */
    /* Infinite loop */
    while (1)
    {
        Button_Loop();
    }
    /* USER CODE END StartButtonTask */
}

void StartLedTask(void *argument)
{
    /* USER CODE BEGIN StartLedTask */
    /* Infinite loop */
    while (1)
    {
        Led_Loop();
    }
    /* USER CODE END StartLedTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
