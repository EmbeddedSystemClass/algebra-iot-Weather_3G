
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
#include <string.h>
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
#include "ATCmdParser.h"
    
#include "Version.h"


/* Private variables ---------------------------------------------------------*/
int32_t stdin_uart_handle;
int32_t stdout_uart_handle;
int32_t stderr_uart_handle;
osThreadId ledTaskHandle;
osThreadId watchdogTaskHandle;
osThreadId echoTaskHandle;
static int32_t modem_uart_handle = -1;
ATCmdParser *modem_parser_handle = NULL;


/* Private function prototypes -----------------------------------------------*/
void LedTask(void const * argument);
void WatchdogTask(void const * argument);
void stdio_init(void);
void EchoTask(void const * argument);

static void modem_init();
static void modem_power_up();
static bool modem_attach();
static int modem_all();
static void send_cmd_modem(char* , char);

//#define modem_send(command, �) atparser_send(modem_parser_handle, command, ##__VA_ARGS__)
//#define modem_recv(response, �) atparser_recv(modem_parser_handle, response, ##__VA_ARGS__)


/**
  * @brief  The application entry point.
  *
  * @retval None
  */
int main(void)
{
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
  osThreadDef(ledTask, LedTask, osPriorityNormal, 0, 512);
  ledTaskHandle = osThreadCreate(osThread(ledTask), NULL);
  
  osThreadDef(watchdogTask, WatchdogTask, osPriorityHigh, 0, 512);
  watchdogTaskHandle = osThreadCreate(osThread(watchdogTask), NULL);

  osThreadDef(echoTask, EchoTask, osPriorityHigh, 0, 512);
  echoTaskHandle = osThreadCreate(osThread(echoTask), NULL);

  ssLoggingPrint(ESsLoggingLevel_Info, 0, "Start OS.");

  /* Start scheduler */
  osKernelStart();
  
  /* We should never get here as control is now taken by the scheduler */
  configASSERT(0);
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


/* LedTask function */
void LedTask(void const * argument)
{

  ssLoggingPrint(ESsLoggingLevel_Info, 0, "LedTask started");
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_11);
    osDelay(1000);
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_11);
    osDelay(1000);
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_11);
    osDelay(1000);
    HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_11);
    osDelay(1000);
	HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_11);
	osDelay(1000);
	HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_11);
	osDelay(1000);
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


void EchoTask(void const * argument)
{
	modem_all();
/*  ssLoggingPrint(ESsLoggingLevel_Info, 0, "EchoTask started");

  modem_init();

  if(modem_attach())
  {
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "Modem attached successfully.");
  }

  /* Infinite loop */
  for(;;)
  {
    osDelay(100);
  }
}



static void modem_init()
{
  ssUartConfigType config;
  bool success;

  /* open modem uart */
  config.Mode = UART_MODE_TX_RX;
  config.Parity = UART_PARITY_NONE;
  config.StopBits = UART_STOPBITS_1;
  config.WordLength = UART_WORDLENGTH_8B;
  config.baudrate = 115200;
  config.FlowControl = UART_HWCONTROL_RTS_CTS;
  modem_uart_handle = ssUartOpen(USART3, &config, 1024);
  configASSERT(modem_uart_handle >= 0);

  /* init modem parser */
  modem_parser_handle = atparser_create(modem_uart_handle);
  configASSERT(modem_parser_handle);

  /* power up modem */
  for(int i=1; !success && i<=10; i++)
  {
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "Power up modem.");
	modem_power_up();
	osDelay(1000*i);
	success = atparser_send(modem_parser_handle, "AT") && atparser_recv(modem_parser_handle, "OK");
	if(success)
	{
	  ssLoggingPrint(ESsLoggingLevel_Info, 0, "Modem powered up successfully.");
	}
	else
	{
      ssLoggingPrint(ESsLoggingLevel_Warning, 0, "Modem power up failed.");
	}
  }
  configASSERT(success);
}


static void modem_power_up()
{
  /* power up modem */
  HAL_GPIO_WritePin(MDM_RESET_GPIO_Port, MDM_RESET_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, GPIO_PIN_RESET);
  osDelay(50);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, GPIO_PIN_SET);
  osDelay(10);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, GPIO_PIN_RESET);
  osDelay(150);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, GPIO_PIN_SET);
  osDelay(100);
}

/*
static bool modem_attach()
{
  bool ret = false;
  int status;
  if(atparser_send(modem_parser_handle, "AT+CGATT?") && atparser_recv(modem_parser_handle, "+CGATT: %d\n", &status))
  {
	  if(status == 1)
	  {
		ret = true;
	  }
	  else
	  {
		  if(atparser_send(modem_parser_handle, "AT+CGATT=1") && atparser_recv(modem_parser_handle, "OK"))
		  {
			  if(atparser_send(modem_parser_handle, "AT+CGATT?") && atparser_recv(modem_parser_handle, "+CGATT: %d\n", &status))
			  {
				  ret = true;
			  }
		  }
		  else
		  {
			  ssLoggingPrint(ESsLoggingLevel_Warning, 0, "Modem attach command failed.");
		  }
	  }
  }
  else
  {
	  ssLoggingPrint(ESsLoggingLevel_Warning, 0, "Modem attach status query command failed.");
  }

  return ret;
}
*/
static void send_cmd_modem(char* cmd, char reicv)
{
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "TEST");
	ssLoggingPrint(ESsLoggingLevel_Info, 0, cmd);
	char buffer[100];
	char ct[100];
	int status;

	atparser_send(modem_parser_handle, cmd);
	atparser_recv(modem_parser_handle, reicv, &status);
	atparser_read(modem_parser_handle,buffer,100);
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "TEST 2");
	strcat(ct,"CMD ");
	strcat(ct,cmd);
	ssLoggingPrint(ESsLoggingLevel_Info, 0, ct);
	ssLoggingPrint(ESsLoggingLevel_Info, 0, buffer);
	memset(ct,'\0',100);
	memset(buffer,'\0',100);

}
static int modem_all(){
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "Modem_All_0");
	 modem_init();
	 if(/*modem_attach()*/ 1 == 1){
		ssLoggingPrint(ESsLoggingLevel_Info, 0, "Modem attached successfully.");
		ssLoggingPrint(ESsLoggingLevel_Info, 0, "Modem_All_1");
		char cmd_AT[][36] = {"AT","AT+CPIN?","AT+CREG?","AT+CREG=1", "AT+CREG?","AT+CGDCONT=1,\"IP\",\"internet.tele2.hr\"","AT+CGACT=1","AT+CGPADDR=1","AT+upsd=0,1,\"hologram\"","AT+upsda=0,3","AT+usocr=17,1000"};
		for(int i = 0; i < 11; i += 1){
		/*	ssLoggingPrint(ESsLoggingLevel_Info, 0, "454545");
			ssLoggingPrint(ESsLoggingLevel_Info, 0, cmd_AT[i]);*/
			send_cmd_modem(cmd_AT[i],'A');
		}
		send_cmd_modem("AT+USOST=0,\"142.93.104.222\",9000,13,\"Testna_poruka\"",'A');
		ssLoggingPrint(ESsLoggingLevel_Info, 0, "Modem_All_2");
	 } else {
		 ssLoggingPrint(ESsLoggingLevel_Info, 0, "Modem_All_3");
	 }
	 return 1;

}
#if 0
static void template()
{

}
#endif


/**
  * @brief  This function is executed in case of error occurrence.
  * @param  file: The file name as string.
  * @param  line: The line in file as a number.
  * @retval None
  */
void _Error_Handler(char *file, int line)
{
  /* User can add his own implementation to report the HAL error return state */
  while(1)
  {
  }
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