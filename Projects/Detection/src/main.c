/******************************************************************************
 * @file main.c
 * @ Luos
 * @ version 1.0.0
******************************************************************************/
#include <main.h>

//driver include

//luos include
#include "luos.h"
#include <luosHAL.h>

//module include
#include "Detection.h"

void SystemClock_Config(void);
/******************************************************************************
 * @brief
 *   User loop function.
 * @Param
 *
 * @Return
 *
 ******************************************************************************/
int main(void)
{
    /////////////////////////////////////////
    //////////////////HW Init////////////////
    /////////////////////////////////////////
    HAL_Init();
    SystemClock_Config();

    /////////////////////////////////////////
    //////////////////FW Init////////////////
    /////////////////////////////////////////
    //Driver Init
    GPIO_Setup();

    //Luos Init
    Luos_Init();

    //Module Init
    Detection_Init();

    /////////////////////////////////////////
    //////////////////FW Loop////////////////
    /////////////////////////////////////////
    while (1)
    {
        //Luos Loop
        Luos_Loop();

        //Module Loop
        Detection_Loop();
    }
}
/*******************************************************************************
 * @brief  SystemClock_Config
*******************************************************************************/
void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;

	/* Select HSI48 Oscillator as PLL source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48;
	RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI48;
	RCC_OscInitStruct.PLL.PREDIV = RCC_PREDIV_DIV2;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct)!= HAL_OK)
	{
		while(1);
	}

	/* Select PLL as system clock source and configure the HCLK and PCLK1 clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1)!= HAL_OK)
	{
		while(1);
	}
}
/******************************************************************************
 * @brief
 *
 * @Param
 *
 * @Return
 *
 ******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
