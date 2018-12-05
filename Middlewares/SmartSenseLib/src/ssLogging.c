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
#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "ssLogging.h"
#include "ssTask.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*
 *  Defines for print prefix formatting.
 */
#define TIME         "%07lu.%03lu"
#define PID          "%04lx/"
#define SEVERITY     "%3s:"
#define DELIMITER    " "

#define LOG_MAX_PRINT_LEN   128

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
char logBufferPtr[LOG_MAX_PRINT_LEN];

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

static uint32_t formatPrintPrefixString(const ESsLoggingLevel level, char *bufptr);

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
  uint32_t length = 0;
  
  if( !(level > ESsLoggingLevel_NotValid &&
        level < ESsLoggingLevel_NoPrints) )
  {
    return;
  }


  osMutexWait(ssLoggingMutex, osWaitForever);
  length = formatPrintPrefixString(level, logBufferPtr);

  va_start(args, unformattedStringPtr);
  length += vsnprintf(logBufferPtr + length,
                      LOG_MAX_PRINT_LEN - length,
                      unformattedStringPtr,
                      args);
  va_end(args);
  printf("%s\r\n", logBufferPtr);
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
  uint32_t length = 0;
  char bufferPtr[LOG_MAX_PRINT_LEN];

  if( !(level > ESsLoggingLevel_NotValid &&
        level < ESsLoggingLevel_NoPrints) )
  {
    return;
  }

  length = formatPrintPrefixString(level, bufferPtr);

  osMutexWait(ssLoggingMutex, osWaitForever);
  va_start(args, unformattedStringPtr);
  length += vsnprintf(logBufferPtr + length,
                      LOG_MAX_PRINT_LEN - length,
                      unformattedStringPtr,
                      args);
  va_end(args);

  printf("%s %3d \"", logBufferPtr, len);
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


#define MEMORY_DUMP_BYTES_IN_LINE    16
#define MEMORY_DUMP_GROUPS_IN_LINE   4

void ssLoggingPrintHD(const ESsLoggingLevel level,
                      const uint32_t category,
                      const uint8_t* src,
                      int len,
                      const char* unformattedStringPtr,
                      ...)
{
  va_list args;
  uint32_t length = 0;
  char bufferPtr[LOG_MAX_PRINT_LEN];
  int i = 0;

  if( !(level > ESsLoggingLevel_NotValid &&
        level < ESsLoggingLevel_NoPrints) )
  {
    return;
  }

  length = formatPrintPrefixString(level, bufferPtr);

  va_start(args, unformattedStringPtr);
  length += vsnprintf(bufferPtr + length,
                      LOG_MAX_PRINT_LEN - length,
                      unformattedStringPtr,
                      args);
  va_end(args);

  printf("%s ADDRESS:%p SIZE:%d\n", bufferPtr, src, len);

  while (i < len)
  {
      int nbytes;
      uint8_t array[MEMORY_DUMP_BYTES_IN_LINE];
      int j;

      nbytes = ((len - i) > MEMORY_DUMP_BYTES_IN_LINE) ? MEMORY_DUMP_BYTES_IN_LINE : (len-i);
      memcpy(array, src+i, nbytes);

      /* Add hex dump */
      printf("  %04x: ", i);
      for (j = 0 ; j < nbytes; j++)
      {
        if (j% MEMORY_DUMP_GROUPS_IN_LINE == 0)
        {
          /* Add extra space to make byte groups. */
          printf(" ");
        }
        printf("%02X", array[j]);

      }

      /* Add ASCII dump. */
      for(j = (MEMORY_DUMP_BYTES_IN_LINE - nbytes); j > 0; j-- )
      {
        /* First need to add spaces if not full line. */
        if ((j%MEMORY_DUMP_GROUPS_IN_LINE) == 0)
        {
          printf("   ");
        }
        else
        {
          printf("  ");
        }
      }

      printf(" | ");
      for (j = 0 ; j < nbytes; j++)
      {
        if (isprint(array[j]))
            printf("%c", array[j]);
        else
            printf(".");
      }
      printf("\n\r");
      i += 16;
  }
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

static uint32_t formatPrintPrefixString(const ESsLoggingLevel level, char *bufptr)
{
  uint32_t timestamp;
  uint32_t length;
  uint32_t seconds;
  uint32_t milliseconds;
  uint32_t task;

  //ss_LocalTimeGet(&timestamp);
  timestamp = OS_TICKS_TO_MILLISECONDS(xTaskGetTickCount());
  seconds = timestamp / 1000;
  milliseconds = timestamp % 1000;
  task = (uint32_t)uxTaskGetCurrentTaskId();

  length = snprintf(bufptr,
                    LOG_MAX_PRINT_LEN,
                    TIME DELIMITER PID SEVERITY DELIMITER,
                    seconds,
                    milliseconds,
                    task,
                    loggingLevelStringTable[level]);

  return length;
}




 
