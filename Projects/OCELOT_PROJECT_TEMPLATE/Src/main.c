
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * Copyright (c) 2018 STMicroelectronics International N.V. 
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <assert.h>

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "bsp.h"
#include "ssUart.h"
#include "ssCli.h"
#include "CliCommonCmds.h"
#include "ssSysCom.h"
#include "ssSupervision.h"
#include "ssLogging.h"
#include "ssDevMan.h"
    
#include "Version.h"


/* Private variables ---------------------------------------------------------*/
int32_t stdin_uart_handle;
int32_t stdout_uart_handle;
int32_t stderr_uart_handle;
osThreadId ledTaskHandle;
osThreadId watchdogTaskHandle;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void LedTask(void const * argument);
void WatchdogTask(void const * argument);

void stdio_init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  BSP_init();
  
  /* Init library modules */
  ssUartInit(BSP_UART_COUNT);
  ssLoggingInit();
  stdio_init();
  ssSysComInit();
  
  ssLoggingPrint(ESsLoggingLevel_Info, 0, "%s running on %s, version %s, built %s by %s", app_name, board_name, build_version, build_date, build_author);
  ssLoggingPrint(ESsLoggingLevel_Info, 0, "Initializing tasks...");
  ssCliUartConsoleInit();
  
  /* start CLI console */
  //ssCliUartConsoleStart();
  //CliCommonCmdsInit();
    
  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(Task, LedTask, osPriorityNormal, 0, 512);
  ledTaskHandle = osThreadCreate(osThread(Task), NULL);
  
  osThreadDef(watchdogTask, WatchdogTask, osPriorityHigh, 0, 512);
  watchdogTaskHandle = osThreadCreate(osThread(watchdogTask), NULL);

  ssLoggingPrint(ESsLoggingLevel_Info, 0, "Start OS.");

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

}

void stdio_init(void)
{
  ssUartConfigType config;

  config.baudrate = STDOUT_UART_BAUDRATE;
  config.FlowControl = UART_HWCONTROL_NONE;
  config.Mode = UART_MODE_TX_RX;
  config.Parity = UART_PARITY_NONE;
  config.StopBits = UART_STOPBITS_1;
  config.WordLength = UART_WORDLENGTH_8B;
  
  stdout_uart_handle = ssUartOpen(STDOUT_UART, &config, STDOUT_USART_BUFFER_SIZE);
  configASSERT(stdout_uart_handle >= 0);
  stderr_uart_handle = stdout_uart_handle;
}


/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/* LedTask function */
void LedTask(void const * argument)
{

  ssLoggingPrint(ESsLoggingLevel_Info, 0, "LedTask started");
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_11);
    osDelay(1000);
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_12);
    osDelay(1000);
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_13);
    osDelay(1000);
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_11);
    osDelay(1000);
	HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_12);
	osDelay(1000);
	HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_13);
	osDelay(1000);
    ssLoggingPrint(ESsLoggingLevel_Info, 0, "Blinky is alive.");
  }
}


void WatchdogTask(void const * argument)
{

  ssLoggingPrint(ESsLoggingLevel_Info, 0, "WatchdogTask started");
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(BSP_WD_GPIO_PORT, BSP_WD_PIN);
    osDelay(100);
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
