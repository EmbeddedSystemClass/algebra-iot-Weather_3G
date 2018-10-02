/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

/*------------------------- INCLUDED FILES ************************************/

#ifdef __cplusplus
extern "C" {
#endif
  
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"

#include "ssCO2Sensor.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/
#define CO2_SENSOR_TASK_STACK_SIZE        240
#define CO2_SENSOR_TASK_PRIORITY          4
#define CO2_SENSOR_TASK_NAME              "CO2 Daemon"
#define USART_BUFFER_SIZE                 32
  
#define CO2_SENSOR_UART_BAUDRATE          9600
#define CO2_SENSOR_UART_WORDLENGTH        UART_WORDLENGTH_8B
#define CO2_SENSOR_UART_STOPBITS          UART_STOPBITS_1  
#define CO2_SENSOR_UART_PARITY            UART_PARITY_NONE
#define CO2_SENSOR_UART_MODE              UART_MODE_TX_RX
#define CO2_SENSOR_UART_FLOWCONTROL       UART_HWCONTROL_NONE
  
#define CO2_SENSOR_EPROM_EMPTY            0xFF
#define CO2_SENSOR_NOT_CONFIGURED         0x00
#define CO2_SENSOR_CONFIGURED             0x01
  
#define CO2_SENSOR_MEASURE_ALL            'Q'
#define CO2_SENSOR_MEASURE_CO2_CURRENT    'z'  
#define CO2_SENSOR_MEASURE_CO2_FILTERED   'Z' 
#define CO2_SENSOR_MEASURE_HUMIDITY       'H'
#define CO2_SENSOR_MEASURE_TEMPERATURE    'T' 
  
#define CO2_SENSOR_UART_TIMEOUT           1000 /* milliseconds */
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

typedef struct
{
  int32_t uartId; 
  uint32_t flags;
  uint8_t buffer[USART_BUFFER_SIZE];
} ssCO2SensorConfigDataType;
  
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

static ssCO2SensorConfigDataType *sensorConfigData;

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/
uint32_t isSensorConfigured(void);
void updateConfigureStatus(uint32_t status);
uint32_t configureSensor(void);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
uint32_t ssCO2SensorOpen(USART_TypeDef* USARTx, uint32_t flags)
{
  ssUartConfigType config;
  int32_t uartId; 
  
  
  config.baudrate = CO2_SENSOR_UART_BAUDRATE;
  config.FlowControl = CO2_SENSOR_UART_FLOWCONTROL;
  config.Mode = CO2_SENSOR_UART_MODE;
  config.Parity = CO2_SENSOR_UART_PARITY;
  config.StopBits = CO2_SENSOR_UART_STOPBITS;
  config.WordLength = CO2_SENSOR_UART_WORDLENGTH;
  
  sensorConfigData = pvPortMalloc(sizeof(ssCO2SensorConfigDataType));
  configASSERT(sensorConfigData);
  
  uartId = ssUartOpen(USARTx, &config, USART_BUFFER_SIZE);
  configASSERT(uartId >= 0);
  
  sensorConfigData->uartId = uartId;
  sensorConfigData->flags = flags;
  
  if((flags & SS_CO2_FLAG_FORCE_CONFIGURE) || (isSensorConfigured() == CO2_SENSOR_NOT_CONFIGURED))
  {
    configureSensor();
  }
  
  return SS_CO2_STATUS_OK;
}


uint16_t ssCO2SensorReadInstantCO2(void)
{
  uint32_t value;
  
  ssUartPuts(sensorConfigData->uartId, "z\r\n");
  ssUartGets(sensorConfigData->uartId, sensorConfigData->buffer, USART_BUFFER_SIZE, CO2_SENSOR_UART_TIMEOUT);
  if(sscanf((char const *)sensorConfigData->buffer, " z %5d\r\n", &value) != 1)
  {
    /* error */
    value = 0;
  }
  else
  {
    /* convert value to proper format */
  }
  
  return (uint16_t)value;
}
uint16_t ssCO2SensorReadFilteredCO2(void)
{
  uint32_t value;
  
  ssUartPuts(sensorConfigData->uartId, "Q\r\n");
  ssUartGets(sensorConfigData->uartId, sensorConfigData->buffer, USART_BUFFER_SIZE, CO2_SENSOR_UART_TIMEOUT);
  if(sscanf((char const *)sensorConfigData->buffer, " Z %5d\r\n", &value) != 1)
  {
    /* error */
    value = 0;
  }
  else
  {
    /* convert value to proper format */
  }
  
  return (uint16_t)value;
}

uint32_t ssCO2SensorCalibrate(void)
{
  uint32_t value;
  uint32_t status = SS_CO2_STATUS_ERR;
  ssUartPuts(sensorConfigData->uartId, "G\r\n");
  ssUartGets(sensorConfigData->uartId, sensorConfigData->buffer, USART_BUFFER_SIZE, CO2_SENSOR_UART_TIMEOUT);
  
  if(sscanf((char const *)sensorConfigData->buffer, " G %5d\r\n", &value) == 1)
  {
    status = SS_CO2_STATUS_ERR;
  }
  
  return status;
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

uint32_t isSensorConfigured(void)
{
  uint32_t address;
  uint32_t value;
  uint32_t ret = CO2_SENSOR_NOT_CONFIGURED;
  
  /* 
  From user manual: "Sensor settings are stored in non-volatile memory, so the sensor only has to be configured once.
  It should not be configured every time it is powered up."
  Therefore we use EEPROM loacation 200 to indicate whether sensor was alread configured or not:
  if location empty (255) or 0, sensor is not configured 
  */
  ssUartPuts(sensorConfigData->uartId, "p 00200\r\n");
  ssUartGets(sensorConfigData->uartId, sensorConfigData->buffer, USART_BUFFER_SIZE, CO2_SENSOR_UART_TIMEOUT);
  if(sscanf((char const *)sensorConfigData->buffer, " p %5d %5d\r\n", &address, &value) == 2)
  {
    if((address == 200) && (value == CO2_SENSOR_CONFIGURED))
    {
      ret = CO2_SENSOR_CONFIGURED;
    }
  }
  
  return ret;
}

void updateConfigureStatus(uint32_t status)
{
  uint32_t address;
  uint32_t value;
  
  if(status == CO2_SENSOR_CONFIGURED)
  {
    ssUartPuts(sensorConfigData->uartId, "P 200 1\r\n");
  }
  else
  {
    ssUartPuts(sensorConfigData->uartId, "P 200 1\r\n");
  }
  ssUartGets(sensorConfigData->uartId, sensorConfigData->buffer, USART_BUFFER_SIZE, CO2_SENSOR_UART_TIMEOUT);
  if(sscanf((char const *)sensorConfigData->buffer, " P %5d %5d\r\n", &address, &value) != 2)
  {
  }
  else if((address != 200) || (value != status))
  {
  }
}

uint32_t configureSensor(void)
{
  uint32_t mode;

  /* set polling mode */
  ssUartPuts(sensorConfigData->uartId, "K 2\r\n");
  ssUartGets(sensorConfigData->uartId, sensorConfigData->buffer, USART_BUFFER_SIZE, CO2_SENSOR_UART_TIMEOUT);
  if(sscanf((char const *)sensorConfigData->buffer, " K %5d\r\n", &mode) != 1)
  {
    return SS_CO2_STATUS_ERR;
  }
  else if(mode != 2)
  {
    return SS_CO2_STATUS_ERR;
  }
  
  /* set output fields to: filtered CO2 value only */
  ssUartPuts(sensorConfigData->uartId, "M 4\r\n");
  ssUartGets(sensorConfigData->uartId, sensorConfigData->buffer, USART_BUFFER_SIZE, CO2_SENSOR_UART_TIMEOUT);
  if(sscanf((char const *)sensorConfigData->buffer, " M %5d\r\n", &mode) != 1)
  {
    return SS_CO2_STATUS_ERR;
  }
  else if(mode != 4)
  {
    return SS_CO2_STATUS_ERR;
  }
  
  updateConfigureStatus(CO2_SENSOR_CONFIGURED);
  
  return SS_CO2_STATUS_OK;
}

#ifdef __cplusplus
}
#endif


 