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

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "ssLogging.h"
#include "ssTask.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/



/*------------------------- TYPE DEFINITIONS ---------------------------------*/

static const char* const loggingLevelStringTable[] = 
{"?N?",
 "DBG",
 "INF",
 "WRN",
 "ERR",                                                  
 "!N!"};

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

static osMutexId ssLoggingMutex = NULL;

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

void ssLoggingInit()
{
  osMutexDef_t mutex_def;
  ssLoggingMutex = osMutexCreate(&mutex_def);
  configASSERT(ssLoggingMutex);
}

void ssLoggingPrint(const ESsLoggingLevel level,
                    const uint32_t category, 
                    const char* unformattedStringPtr,
                    ...)
{
  va_list args; 
  uint32_t timestamp;
  uint32_t seconds;
  uint32_t milliseconds;
  uint32_t task;
  
  if( !(level > ESsLoggingLevel_NotValid &&
        level < ESsLoggingLevel_NoPrints) )
  {
    return;
  }

  timestamp = OS_TICKS_TO_MILLISECONDS(xTaskGetTickCount());
  seconds = timestamp / 1000;
  milliseconds = timestamp % 1000;
  task = (uint32_t)uxTaskGetCurrentTaskId();
  
  osMutexWait(ssLoggingMutex, osWaitForever);
  printf("%07u.%03u %d/%s ", seconds, milliseconds, task, loggingLevelStringTable[level]);
  va_start(args, unformattedStringPtr);
  vprintf(unformattedStringPtr, args);
  va_end(args);
  printf("\r\n");
  osMutexRelease(ssLoggingMutex);
}



void ssLoggingPrintRawStr(const ESsLoggingLevel level,
                          const uint32_t category,
                          const char* string, 
                          int len,
                          const char* unformattedStringPtr,
                          ...)
{
  va_list args;
  uint32_t timestamp;
  uint32_t seconds;
  uint32_t milliseconds;
  uint32_t task;

  if( !(level > ESsLoggingLevel_NotValid &&
        level < ESsLoggingLevel_NoPrints) )
  {
    return;
  }

  timestamp = OS_TICKS_TO_MILLISECONDS(xTaskGetTickCount());
  seconds = timestamp / 1000;
  milliseconds = timestamp % 1000;
  task = (uint32_t)uxTaskGetCurrentTaskId();

  osMutexWait(ssLoggingMutex, osWaitForever);
  printf("%07u.%03u %d/%s ", seconds, milliseconds, task, loggingLevelStringTable[level]);
  va_start(args, unformattedStringPtr);
  vprintf(unformattedStringPtr, args);
  va_end(args);

  printf("%3d \"", len);
  while (len --)
  {
    char ch = *string++;
    if ((ch > 0x1F) && (ch < 0x7F))
    { // is printable
      if      (ch == '%')  printf("%%");
      else if (ch == '"')  printf("\\\"");
      else if (ch == '\\') printf("\\\\");
      else printf( "%c", ch);
    } else
    {
      if      (ch == '\a') printf("\\a"); // BEL (0x07)
      else if (ch == '\b') printf("\\b"); // Backspace (0x08)
      else if (ch == '\t') printf("\\t"); // Horizontal Tab (0x09)
      else if (ch == '\n') printf("\\n"); // Linefeed (0x0A)
      else if (ch == '\v') printf("\\v"); // Vertical Tab (0x0B)
      else if (ch == '\f') printf("\\f"); // Formfeed (0x0C)
      else if (ch == '\r') printf("\\r"); // Carriage Return (0x0D)
      else                 printf("\\x%02x", (unsigned char)ch);
    }
  }

  printf("\"\r\n");
  osMutexRelease(ssLoggingMutex);
}


/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

#ifdef __cplusplus
}
#endif


 
