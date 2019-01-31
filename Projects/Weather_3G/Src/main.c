
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
osThreadId windTaskHandle;
osThreadId waterTaskHandle;
osThreadId dhtTaskHandle;

static int32_t modem_uart_handle = -1;
ATCmdParser *modem_parser_handle = NULL;
int32_t hz;
GPIO_PinState state;
uint32_t windChrono;
uint32_t waterChrono;
uint32_t sendData;
uint32_t resetWater			= 3600000;
uint32_t resetWind			= 1000;
volatile uint32_t counter	= 0;
int16_t windSpeed			= 0;
int16_t waterLevel			= 0;
int32_t change				= 0;
int16_t humidityLevel		= 0;
int16_t temperatureLevel	= 0;


uint8_t Rh_byte1, Rh_byte2, Temp_byte1, Temp_byte2;
uint16_t sum;
int temp_low, temp_high, rh_low, rh_high;
char temp_char1[2], temp_char2, rh_char1[2], rh_char2;
uint8_t check = 0;
GPIO_InitTypeDef GPIO_InitStruct;
uint8_t dhtData[5];


/* Private function prototypes -----------------------------------------------*/
void LedTask(void const * argument);
void WatchdogTask(void const * argument);
void stdio_init(void);
void EchoTask(void const * argument);
void WindTask(void const * argument);
void WaterTask(void const * argument);

void DhtTask(void const * argument);
uint8_t read_data (void);
void check_response (void);
void DHT22_start (void);
void set_gpio_output (void);
void set_gpio_input (void);



static uint32_t expectPulseHigh();
static uint32_t expectPulseLow();
static void modem_init();
static void modem_power_up();
static bool modem_attach();
static void modem_start();
static int modem_all();
static void send_cmd_modem(char* , char);
static void read_wind();
static void read_water();
static void read_dht();
static void read_raw();
static void send_sensor();


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
  SysTick_Config(SystemCoreClock /1000);
  ssLoggingPrint(ESsLoggingLevel_Info, 0, "%s running on %s, version %s, built %s by %s", app_name, board_name, build_version, build_date, build_author);
  ssLoggingPrint(ESsLoggingLevel_Info, 0, "Initializing tasks...");
  ssCliUartConsoleInit();
  
  /* start CLI console */
  //ssCliUartConsoleStart();
  //CliCommonCmdsInit();

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  //osThreadDef(ledTask, LedTask, osPriorityNormal, 0, 512);
  //ledTaskHandle = osThreadCreate(osThread(ledTask), NULL);

  osThreadDef(watchdogTask, WatchdogTask, osPriorityHigh, 0, 1024);
  watchdogTaskHandle = osThreadCreate(osThread(watchdogTask), NULL);

  //osThreadDef(echoTask, EchoTask, osPriorityLow, 0, 1024);
  //echoTaskHandle = osThreadCreate(osThread(echoTask), NULL);

  //osThreadDef(windTask, WindTask, osPriorityLow, 0, 1024);
  //windTaskHandle = osThreadCreate(osThread(windTask), NULL);

  //osThreadDef(waterTask, WaterTask, osPriorityLow, 0, 1024);
  //waterTaskHandle = osThreadCreate(osThread(waterTask), NULL);

  osThreadDef(dhtTask, DhtTask, osPriorityLow, 0, 1024);
  dhtTaskHandle = osThreadCreate(osThread(dhtTask), NULL);

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
	//modem_start();
	//for(;;){
		//send_sensor();
	//}
  //ssLoggingPrint(ESsLoggingLevel_Info, 0, "WatchdogTask started");
  /* Infinite loop */
  for(;;)
  {
    HAL_GPIO_TogglePin(BSP_WD_GPIO_PORT, BSP_WD_PIN);
    osDelay(100);
  }
}


void WindTask(void const * argument)
{
	windChrono = HAL_GetTick();
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "WindTask started");
	for(;;)
	  {
		 read_wind();
	  }
}
void WaterTask(void const * argument)
{
	waterChrono = HAL_GetTick();
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "WaterTask started");
	for(;;)
	  {
		 read_water();
	  }
}
void DhtTask(void const * argument)
{
	//waterChrono = HAL_GetTick();
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "dhtTask started");
	for(;;)
	  {
		osDelay (2000);
		ssLoggingPrint(ESsLoggingLevel_Info, 0, "Start loop");
		 read_dht();
		 char size[3];
		 sprintf(size,"%d",humidityLevel);
		 ssLoggingPrint(ESsLoggingLevel_Info, 0, size);

		 sprintf(size,"%d",temperatureLevel);
		 ssLoggingPrint(ESsLoggingLevel_Info, 0, size);
	  }
}
void EchoTask(void const * argument)
{
  ssLoggingPrint(ESsLoggingLevel_Info, 0, "EchoTask started");
  modem_start();
  //state = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2);
  //windChrono = HAL_GetTick();
  sendData = HAL_GetTick();;
  for(;;)
  {
	  //read_raw();
	  //read_dht();
	  //read_wind();
	  //read_water();
	  if(sendData < HAL_GetTick() - 10000){
		  send_sensor();
		  sendData = HAL_GetTick();
	  }
	  //osDelay(100);
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
static void send_sensor(){
	char wind[20];
	sprintf(wind,"%s%d","'wind' : ",windSpeed);

	char water[20];
	sprintf(water,"%s%d","'precipitation' : ",waterLevel);

	char humidity[20];
	sprintf(humidity,"%s%d","'humidity' : ", humidityLevel);

	char temperature[20];
	sprintf(temperature,"%s%d","'temperature' : ",temperatureLevel);

	char json[100];
	sprintf(json, "\"{%s,%s,%s,%s}\"", wind,water,humidity,temperature);

	char finalData[200];
	char part1[35] = "AT+USOST=0,\"142.93.104.222\",9000,";


	char size[3];
	sprintf(size,"%d",strlen(json)-2);
	sprintf(finalData,"%s%s,%s",part1,size,json);

	ssLoggingPrint(ESsLoggingLevel_Info, 0, finalData);
	send_cmd_modem(finalData,'A');

	send_cmd_modem(json,'A');
	send_cmd_modem("AT+USORF=0,255",'A');
}
static void send_cmd_modem(char* cmd, char reicv)
{
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "Command");
	ssLoggingPrint(ESsLoggingLevel_Info, 0, cmd);
	char buffer[100];
	char ct[100];

	memset(ct,'\0',100);
	memset(buffer,'\0',100);

	atparser_send(modem_parser_handle, cmd);
	//osDelay(10000);
	atparser_recv(modem_parser_handle, reicv);
	atparser_read(modem_parser_handle,buffer,100);

	ssLoggingPrint(ESsLoggingLevel_Info, 0, "TEST 2");
	strcat(ct,"CMD ");
	strcat(ct,cmd);
	ssLoggingPrint(ESsLoggingLevel_Info, 0, ct);
	ssLoggingPrint(ESsLoggingLevel_Info, 0, buffer);


}
static void modem_start(){
	 modem_init();
	 if(modem_attach()){
		char cmd_AT[][50] = {"AT","AT+CPIN?","AT+CREG?","AT+CREG=1", "AT+CREG?", "AT+CGDCONT=1,\"IP\",\"internet.tele2.hr\"", "AT+CGACT=1", "AT+CGPADDR=1", "AT+upsd=0,1,\"hologram\"", "AT+upsda=0,3", "AT+usocr=17,1000"};
		for(int i = 0; i < 11; i += 1){
			//ssLoggingPrint(ESsLoggingLevel_Info, 0, "454545");
			//ssLoggingPrint(ESsLoggingLevel_Info, 0, cmd_AT[i]);
			send_cmd_modem(cmd_AT[i],'A');
		}
	 }
}
static int modem_all(){
	 modem_init();
	 if(modem_attach()){
		char cmd_AT[][50] = {"AT","AT+CPIN?","AT+CREG?","AT+CREG=1", "AT+CREG?", "AT+CGDCONT=1,\"IP\",\"internet.tele2.hr\"", "AT+CGACT=1", "AT+CGPADDR=1", "AT+upsd=0,1,\"hologram\"", "AT+upsda=0,3", "AT+usocr=17,1000"};
		for(int i = 0; i < 11; i += 1){
			//ssLoggingPrint(ESsLoggingLevel_Info, 0, "454545");
			//ssLoggingPrint(ESsLoggingLevel_Info, 0, cmd_AT[i]);
			send_cmd_modem(cmd_AT[i],'A');
		}
		for(;;){
			char finalData[100];
			char part1[35] = "AT+USOST=0,\"142.93.104.222\",9000,";
			char str[16];
			sprintf(str, "%d", windSpeed);

			char size[2];
			sprintf(size,"%d",sizeof(str)/sizeof(char));
			sprintf(finalData,"%s%s,%s",part1,size,str);

			ssLoggingPrint(ESsLoggingLevel_Info, 0, finalData);
			send_cmd_modem(finalData,'A');
			send_cmd_modem("AT+USORF=0,255",'A');
		}

		ssLoggingPrint(ESsLoggingLevel_Info, 0, "Modem_All_2");
	 } else {
		 ssLoggingPrint(ESsLoggingLevel_Info, 0, "Modem_All_3");
	 }
	 return 1;

}
static void read_wind(){

	GPIO_PinState currentState = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2);

	if(state != currentState){
		hz += 1;
		state = currentState;
	}
	if(windChrono < HAL_GetTick() - resetWind){
		windSpeed = hz;
		windChrono = HAL_GetTick();

		char size[3];
		sprintf(size,"%d",windSpeed);
		ssLoggingPrint(ESsLoggingLevel_Info, 0, size);
		hz = 0;
	}
	//char size[3];
	//sprintf(size,"%d",hz);
	//ssLoggingPrint(ESsLoggingLevel_Info, 0, size);

}
static void read_water(){
	GPIO_PinState currentState = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_3);

	if(currentState != 1){
		change += 1;
		if(change == 1){
			waterLevel += 1;
			ssLoggingPrint(ESsLoggingLevel_Info, 0, "Change");
		}
	} else {
		change = 0;
	}
	//One hour 3600000
	if(waterChrono < HAL_GetTick() - resetWater){
		waterLevel = 0;
		waterChrono = HAL_GetTick();
	}
}
void DHT22_start (void)
{
	//set_gpio_output ();  // set the pin as output
	HAL_GPIO_WritePin (GPIOE, GPIO_PIN_2, 0);   // pull the pin low
	osDelay(1);   // wait for 1 ms
	HAL_GPIO_WritePin (GPIOE, GPIO_PIN_2, 1);   // pull the pin high
	//osDelay(30);   // wait for 30 ms
	//set_gpio_input ();   // set as input
}

void check_response (void)
{
	//osDelay(40);
	if (!(HAL_GPIO_ReadPin (GPIOE, GPIO_PIN_2)))
	{
		osDelay(1);
		if ((HAL_GPIO_ReadPin (GPIOE, GPIO_PIN_2))) check = 1;
	}
	while ((HAL_GPIO_ReadPin (GPIOE, GPIO_PIN_2)));   // wait for the pin to go low
}

uint8_t read_data (void)
{
	uint8_t i,j;
	for (j=0;j<8;j++)
	{
		while (!(HAL_GPIO_ReadPin (GPIOE, GPIO_PIN_2)));   // wait for the pin to go high
		osDelay(40);   // wait for 40 ms
		if ((HAL_GPIO_ReadPin (GPIOE, GPIO_PIN_2)) == 0)   // if the pin is low
		{
			i&= ~(1<<(7-j));   // write 0
		}
		else i|= (1<<(7-j));  // if the pin is high, write 1
		while ((HAL_GPIO_ReadPin (GPIOE, GPIO_PIN_2)));  // wait for the pin to go low
	}
	return i;
}

/*void set_gpio_output (void)
{
	//Configure GPIO pin output: PA1
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}*/

/*void set_gpio_input (void)
{
	//Configure GPIO pin input: PA1
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
}*/

static void read_dht(){
	DHT22_start ();
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "1");
	check_response ();
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "2");
	Rh_byte1 = read_data ();
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "3");
	Rh_byte2 = read_data ();
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "4");
	Temp_byte1 = read_data ();
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "5");
	Temp_byte2 = read_data ();
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "6");
	sum = read_data();
	ssLoggingPrint(ESsLoggingLevel_Info, 0, "7");
	//if (sum == (Rh_byte1+Rh_byte2+Temp_byte1+Temp_byte2))
	{
		temperatureLevel = ((Temp_byte1<<8)|Temp_byte2);
		humidityLevel = ((Rh_byte1<<8)|Rh_byte2);
	}

	temp_low = temperatureLevel/10;
	temp_high = temperatureLevel%10;

	rh_low = humidityLevel/10;
	rh_high = humidityLevel%10;
	osDelay(1000);
	//Code from arduino
	//GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
	//void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
	/*dhtData[0] = dhtData[1] = dhtData[2] = dhtData[3] = dhtData[4] = 0;

	// start the reading process.
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET);
	osDelay(250);

	 // First set data line low for 20 milliseconds.
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET);
	osDelay(20);

	uint32_t cycles[80];
	  {
	    // End the start signal by setting data line high for 40 microseconds.
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET);
	    osDelay(40);

	    // Now start reading the data line to get the value from the DHT sensor.
	    //pinMode(_pin, INPUT_PULLUP);
	    osDelay(10);  // Delay a bit to let sensor pull data line low.

	    // First expect a low signal for ~80 microseconds followed by a high signal
	    // for ~80 microseconds again.
	    if (expectPulseLow() == 0) {
	    	ssLoggingPrint(ESsLoggingLevel_Info, 0, "Timeout waiting for start signal low pulse.");
	    }
	    if (expectPulseHigh() == 0){
	    	ssLoggingPrint(ESsLoggingLevel_Info, 0, "Timeout waiting for start signal high pulse.");
	    }

	    // Now read the 40 bits sent by the sensor.  Each bit is sent as a 50
	    // microsecond low pulse followed by a variable length high pulse.  If the
	    // high pulse is ~28 microseconds then it's a 0 and if it's ~70 microseconds
	    // then it's a 1.  We measure the cycle count of the initial 50us low pulse
	    // and use that to compare to the cycle count of the high pulse to determine
	    // if the bit is a 0 (high state cycle count < low state cycle count), or a
	    // 1 (high state cycle count > low state cycle count). Note that for speed all
	    // the pulses are read into a array and then examined in a later step.
	    for (int i=0; i<80; i+=2) {
	      cycles[i]   = expectPulseLow();
	      cycles[i+1] = expectPulseHigh();
	    }
	    for (int i=0; i<40; ++i) {
	    	uint32_t lowCycles  = cycles[2*i];
	    	uint32_t highCycles = cycles[2*i+1];
	    	if ((lowCycles == 0) || (highCycles == 0)) {
	    		ssLoggingPrint(ESsLoggingLevel_Info, 0, "Timeout waiting for pulse.");
	    	}
	    	dhtData[i/8] <<= 1;
	    	// Now compare the low and high cycle times to see if the bit is a 0 or 1.
	    	if (highCycles > lowCycles) {
	    		// High cycles are greater than 50us low cycle count, must be a 1.
	    		dhtData[i/8] |= 1;
	    	}
	    } // Timing critical code is now complete.
	    if (dhtData[4] == ((dhtData[0] + dhtData[1] + dhtData[2] + dhtData[3]) & 0xFF)) {
	    	ssLoggingPrint(ESsLoggingLevel_Info, 0, "Checksum done");
	    }
	    else {
	    	ssLoggingPrint(ESsLoggingLevel_Info, 0, "Checksum fail.");
	    }
	    humidityLevel = 0;
	    humidityLevel = dhtData[0];
	    //humidityLevel *= 256;
	    //humidityLevel += dhtData[1];
	    //humidityLevel *= 0.1;

	    temperatureLevel = dhtData[2] & 0x7F;
	    //temperatureLevel *= 256;
		temperatureLevel += dhtData[3];
		//temperatureLevel *= 0.1;
		if (dhtData[2] & 0x80) {
			temperatureLevel *= -1;
		}
	  }*/
}
static void read_raw(){
	GPIO_PinState currentState = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2);
	if(currentState){
		ssLoggingPrint(ESsLoggingLevel_Info, 0, "True");
	} else {
		ssLoggingPrint(ESsLoggingLevel_Info, 0, "False");
	}
}
static uint32_t expectPulseHigh(){
	uint32_t maxCycles = 1000;
	uint32_t count = 0;
	GPIO_PinState currentState = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2);
	while (!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2)) {
	    if (count++ >= maxCycles) {
	    	return 0; // Exceeded timeout, fail.
	    }
	}
	return count;
}
static uint32_t expectPulseLow(){
	uint32_t maxCycles = 1000;
	uint32_t count = 0;
	GPIO_PinState currentState = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2);
	while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2)) {
	    if (count++ >= maxCycles) {
	    	return 0; // Exceeded timeout, fail.
	    }
	}
	return count;
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
