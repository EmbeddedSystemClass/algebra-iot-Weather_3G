/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2018. All rights reserved.
 *
 **/

/*------------------------- INCLUDED FILES ************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOSConfig.h"
#include <string.h>
#include <stdbool.h>
#include <time.h>

#include "ssGNSS.h"
#include <ssLogging.h>

/*------------------------- MACRO DEFINITIONS --------------------------------*/
#define USART_BUFFER_SIZE           72

#define GNSS_EN_Pin GPIO_PIN_8
#define GNSS_EN_GPIO_Port GPIOC

#define GNSS_UART_BAUDRATE          9600
#define GNSS_UART_WORDLENGTH        UART_WORDLENGTH_8B
#define GNSS_UART_STOPBITS          UART_STOPBITS_1
#define GNSS_UART_PARITY            UART_PARITY_NONE
#define GNSS_UART_MODE              UART_MODE_RX
#define GNSS_UART_FLOWCONTROL       UART_HWCONTROL_NONE

#define GNSS_UART_TIMEOUT           1000 /* milliseconds */

#define GNSS_NOT_CONFIGURED         0x00
#define GNSS_CONFIGURED             0x01

#define TO_DECIMAL(x)               (int)(x)/100 + ((((int)x)%100) + (x - (int)x))/60
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef struct
{
  int32_t uartId; 
  uint32_t flags;
  uint8_t buffer[USART_BUFFER_SIZE];
} ssGNSSConfigDataType;


/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

static ssGNSSConfigDataType *GNSSConfigData;

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
uint8_t ssGNSSinit(USART_TypeDef *USARTx, uint32_t flags)
{
    ssUartConfigType config;
    int32_t uartId;

    config.baudrate = GNSS_UART_BAUDRATE;
    config.FlowControl = GNSS_UART_FLOWCONTROL;
    config.Mode = GNSS_UART_MODE;
    config.Parity = GNSS_UART_PARITY;
    config.StopBits = GNSS_UART_STOPBITS;
    config.WordLength = GNSS_UART_WORDLENGTH;

    GNSSConfigData = pvPortMalloc(sizeof(ssGNSSConfigDataType));
    configASSERT(GNSSConfigData);

    uartId = ssUartOpen(USARTx, &config, USART_BUFFER_SIZE);

    GNSSConfigData->uartId = uartId;
    GNSSConfigData->flags = flags;

    HAL_GPIO_WritePin(GNSS_EN_GPIO_Port, GNSS_EN_Pin, GPIO_PIN_SET);


    return SS_GNSS_STATUS_OK;
}

void ssGNSSRead(void)
{
    while(1)
    {
      ssUartGets(GNSSConfigData->uartId, GNSSConfigData->buffer, USART_BUFFER_SIZE, GNSS_UART_TIMEOUT);
      ssLoggingPrint(ESsLoggingLevel_Info, 0, "%s", GNSSConfigData->buffer);
    }
} 

bool ssGNSSGetCoords(struct ssGNSScoords *coords)
{
    do
    {
      ssUartGets(GNSSConfigData->uartId, GNSSConfigData->buffer, USART_BUFFER_SIZE, GNSS_UART_TIMEOUT);
    } while(sscanf((char const *)GNSSConfigData->buffer, "$GNGLL,%f,%*c,%f,", &(coords->lat), &(coords->lon)) != 2);
    
    coords->lat = TO_DECIMAL(coords->lat);
    coords->lon = TO_DECIMAL(coords->lon);
    ssLoggingPrint(ESsLoggingLevel_Info, 0, "%f : %f", coords->lat, coords->lon);

    return true;
}

/* Function retrieves UTC time and date from GNSS */
bool ssGNSSGetTime(char *datetime, size_t size)
{
    char time[7];
    char date[7];
    char *p;
    do {
        ssUartGets(GNSSConfigData->uartId, GNSSConfigData->buffer, USART_BUFFER_SIZE, GNSS_UART_TIMEOUT);
    } while(sscanf((char const *)GNSSConfigData->buffer, "$GNRMC,%6[0-9].%*d,%*c,%*f,%*c,%*f,%*c,%*f,,%6[0-9]*", time, date) != 2);
    
    p = time;
    for (uint8_t i = 0; i < 20; i++)
    {
        if (i < 8)
        {
            if (i == 2 || i == 5)
            {
                datetime[i] = ':';
            }
            else
            {
                datetime[i] = *p;
                p++;
            }
        }
        if (i == 8)
        {
            p = date;
            datetime[i] = ' ';
        }
        if (i > 8)
        {
            if (i == 11 || i == 14)
            {
                datetime[i] = '.';
            }
            else if (i == 15)
            {
                datetime[i] = '2';
            }
            else if (i == 16)
            {
                datetime[i] = '0';
            }
            else
            {
                datetime[i] = *p;
                p++;
            }
        }
    }
    ssLoggingPrint(ESsLoggingLevel_Info, 0, "%s", datetime);

    return true;
}

/* Convert datetime format to unix timestamp */

uint64_t datetime_to_timestamp(const char *datetime)
{
    struct tm t;
    time_t seconds;
    uint32_t ts_year, ts_month, ts_day, ts_hour, ts_min, ts_sec = 0;
    ts_year = ts_month = ts_day = ts_hour = ts_min = ts_sec;

    if (sscanf(datetime, "%d:%d:%d %d.%d.%d", &ts_hour, &ts_min, &ts_sec, &ts_day, &ts_month, &ts_year) != 6)
    {
        return 0;
    }

    t.tm_year = ts_year-1900;
    t.tm_mon = ts_month-1;       
    t.tm_mday = ts_day-1;          
    t.tm_hour = ts_hour;
    t.tm_min = ts_min;
    t.tm_sec = ts_sec;
    t.tm_isdst = -1;        
    seconds = mktime(&t);


    return seconds;
}
