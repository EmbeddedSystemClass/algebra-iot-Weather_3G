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

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
 
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "task.h"
   
#include "ssUart.h"
#include "ssCli.h"
  
/*------------------------- MACRO DEFINITIONS --------------------------------*/
  
#define CLI_TASK_STACK_SIZE        240
#define CLI_TASK_PRIORITY          4
#define CLI_TASK_NAME              "CLI"
 
#if defined(CLI_CONSOLE_UART)
#define CLI_UART                CLI_CONSOLE_UART
#else 
#define CLI_UART                USART3
#endif
#if defined(CLI_CONSOLE_UART_BAUDRATE)
#define CLI_UART_BAUDRATE       CLI_CONSOLE_UART_BAUDRATE
#else
#define CLI_UART_BAUDRATE       115200
#endif
#define CLI_UART_WORDLENGTH     UART_WORDLENGTH_8B
#define CLI_UART_STOPBITS       UART_STOPBITS_1  
#define CLI_UART_PARITY         UART_PARITY_NONE
#define CLI_UART_MODE           UART_MODE_TX_RX
#define CLI_UART_FLOWCONTROL    UART_HWCONTROL_NONE
#define CLI_USART_BUFFER_SIZE   32
  
#define ASCII_CTRL_C  (0x03)  
#define ASCII_DEL		  (0x7F)
#define ASCII_ESC     (0x1B)  /* escape character */
#define ASCII_ARW     (0x5B)  /* [ - arrows are represented as ESC, [, A/B/C/D */

#define CLI_INPUT_BUFFER_SIZE 80  
#define CLI_OUTPUT_BUFFER_SIZE 80
 
#define CLI_INPUT_HISTORY   4
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/
 
__weak const char cliAppName[] = "Unnamed";
const char * const cliSubcommandErrStr = "Missing or invalid subcommand! Check help.\n\r";  
const char * const cliParameterErrStr = "Missing or invalid parameter(s)! Check help.\n\r"; 
const char * const cliParamValueErrStr = "Parameter out of range!\n\r"; 

/*------------------------- PRIVATE VARIABLES --------------------------------*/

static const char * const newLine = "\r\n";
static const char * const clearScreen = "\033[2J\033[1;1H";

static char cliInputBuffer[CLI_INPUT_BUFFER_SIZE];
static char cliOutputBuffer[CLI_OUTPUT_BUFFER_SIZE];
static char *cliInputHistory[CLI_INPUT_HISTORY];

int32_t cliUart; 


/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

static void ssCliUartConsoleTask(void *argument);
static void printWelcomemessage(void);



/* Common commands */
#if(configGENERATE_RUN_TIME_STATS == 1)
static BaseType_t CliCommonTopCommand( char *writeBuffer, size_t size, const char *command, const BaseType_t intr);
static BaseType_t CliCommonProcCommand( char *writeBuffer, size_t size, const char *command, const BaseType_t intr);

static const CLI_Command_Definition_t topCmdDesc =
{
  "top",
  "top: prints tasks run time statistics\r\n",
  CliCommonTopCommand,
  0
};

static const CLI_Command_Definition_t procCmdDesc =
{
  "proc",
  "proc: prints tasks' info\r\n",
  CliCommonProcCommand,
  0
};
#endif

static BaseType_t CliCommonHistoryCommand( char *writeBuffer, size_t size, const char *command, const BaseType_t intr);

static const CLI_Command_Definition_t historyCmdDesc =
{
  "history",
  "history: prints command history' info\r\n",
  CliCommonHistoryCommand,
  0
};

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
void ssCliUartConsoleInit(void)
{
  uint32_t i;
  ssUartConfigType config;

#if(configGENERATE_RUN_TIME_STATS == 1)
  FreeRTOS_CLIRegisterCommand(&topCmdDesc);
  FreeRTOS_CLIRegisterCommand(&procCmdDesc);
#endif
  
  FreeRTOS_CLIRegisterCommand(&historyCmdDesc); 
  
  config.baudrate = CLI_UART_BAUDRATE;
  config.FlowControl = CLI_UART_FLOWCONTROL;
  config.Mode = CLI_UART_MODE;
  config.Parity = CLI_UART_PARITY;
  config.StopBits = CLI_UART_STOPBITS;
  config.WordLength = CLI_UART_WORDLENGTH;
  
  cliUart = ssUartOpen(CLI_UART, &config, CLI_USART_BUFFER_SIZE);
  configASSERT(cliUart >= 0);
  
  for(i=0; i<CLI_INPUT_HISTORY; i++)
  {
    cliInputHistory[i] = NULL;
  }
}
  
void ssCliUartConsoleStart(void)
{
  BaseType_t ret;
  
  ret = xTaskCreate(ssCliUartConsoleTask,
                    CLI_TASK_NAME,
                    CLI_TASK_STACK_SIZE, /* Stack depth in words. */
                    NULL, 
                    CLI_TASK_PRIORITY, /* This task will run at priority 1. */
                    NULL /* We are not going to use the task handle. */
                    );
  configASSERT(ret == pdPASS);
}

void ssCliDebugPrint(char* format, ...)
{
#if defined(CLI_DEBUG_PRINT)
  va_list args;
  va_start(args, format);
  vsnprintf(cliOutputBuffer, CLI_OUTPUT_BUFFER_SIZE, format, args);
  va_end(args);
  
  ssUartPuts(cliUart, cliOutputBuffer);
#endif
}
/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

static void printWelcomemessage(void)
{
  ssUartPuts( cliUart, newLine);
  ssUartPuts( cliUart, "SmartSense ");
  ssUartPuts( cliUart, cliAppName);
  ssUartPuts( cliUart, " command server.\r\nType help to view a list of registered commands.\r\n\r\n>");
}

void ssCliUartConsoleTask(void *argument)
{ 
  uint32_t ucInputIndex = 0;
  int32_t historyCurrentIndex = -1; 
  uint32_t updateInputFromHistory = pdFALSE;
  
  memset( cliInputBuffer, 0x00, CLI_INPUT_BUFFER_SIZE );
  memset( cliOutputBuffer, 0x00, CLI_OUTPUT_BUFFER_SIZE );
 
  /* Send the welcome message. */
  ssUartPuts( cliUart, clearScreen);
	printWelcomemessage();
  while(1)
  {
    char c;
    c = ssUartGetc(cliUart, portMAX_DELAY);
    if(c=='\r')
    {
      BaseType_t ret;
      
      /* Start the command output in fresh line */
      ssUartPuts(cliUart, newLine);
      
      if(updateInputFromHistory)
      {
        /* Line from history was being executed, use that one as an input line. */
        strcpy(cliInputBuffer, cliInputHistory[historyCurrentIndex]);
        ucInputIndex = strlen(cliInputBuffer);
      }
      
      /* Pass the received command to the command interpreter.  The
			 * command interpreter is called repeatedly until it returns
			 * pdFALSE	(indicating there is no more output) as it might
			 * generate more than one string. 
       */
      if(cliInputBuffer[0] != '\0')
      {
        BaseType_t interrupted;
        
        /* save current line to history if it is different from the last one */
        if(strcmp(cliInputHistory[0], cliInputBuffer) != 0)
        {
          char *line = NULL;
          uint32_t j;
          
          line = pvPortMalloc(strlen(cliInputBuffer) + 1);
          configASSERT(line);
          strcpy(line, cliInputBuffer);
          
          if(cliInputHistory[CLI_INPUT_HISTORY-1])
          {
            vPortFree(cliInputHistory[CLI_INPUT_HISTORY-1]);
          }
          
          for(j=CLI_INPUT_HISTORY-1; j>0; j--)
          {
            cliInputHistory[j] = cliInputHistory[j-1];
          }
          cliInputHistory[0] = line;
        }
        historyCurrentIndex = -1;
        updateInputFromHistory = pdFALSE;

        interrupted = pdFALSE;
        do
        {         
          if(!interrupted)
          {
            /* Check input for ^C and break current command if pressed.
             * Ignore any other key, user should not write new command while one is executed.
             */
            interrupted = (ssUartGetc(cliUart, 0) == ASCII_CTRL_C);
            if(interrupted)
            {
              /* echo to output */
              ssUartPuts(cliUart, "^C\n\r");
            }
          }
          /* Get the next output string from the command interpreter. */
          ret = FreeRTOS_CLIProcessCommand(cliInputBuffer, cliOutputBuffer, CLI_OUTPUT_BUFFER_SIZE, interrupted);
          /* Write the generated string to the UART. */
          ssUartPuts(cliUart, cliOutputBuffer);
          /* check if command is interrupted */
          
        } while(ret == pdTRUE);
      }
      memset( cliInputBuffer, 0x00, CLI_INPUT_BUFFER_SIZE );
      ucInputIndex = 0;
      ssUartPutc(cliUart, '>');
    }
    else if(c == '\n')
    {
      /* ignore char */
    }
    else if(c == ASCII_ESC)
    {
      if(ssUartGetc(cliUart, portMAX_DELAY) == '[')
      {
        c = ssUartGetc(cliUart, portMAX_DELAY);
        if(c == 'A')
        {
          /* arrow up - move towards older lines */
          if((historyCurrentIndex != (CLI_INPUT_HISTORY - 1)) &&
             (cliInputHistory[historyCurrentIndex+1] != NULL))
          {
            historyCurrentIndex++;
            ssUartPuts(cliUart, "\33[2K\r>");
            ssUartPuts(cliUart, cliInputHistory[historyCurrentIndex]);
            updateInputFromHistory = pdTRUE;
          }
        }
        if(c == 'B')
        {
          /* arrow down - move back to the present */
          /* arrow up - move towards older lines */
          if(historyCurrentIndex != -1)
          {
            historyCurrentIndex--;
            ssUartPuts(cliUart, "\33[2K\r>");
            if(historyCurrentIndex == -1)
            {
              ssUartPuts(cliUart, cliInputBuffer);
              updateInputFromHistory = pdTRUE;
            }
            else
            {
              ssUartPuts(cliUart, cliInputHistory[historyCurrentIndex]);
            }
          }
        }
      }
    }
    else if (c == ASCII_CTRL_C)
    {
      /* echo ctrl-c and go to new line */
      ssUartPuts(cliUart, "^C\n\r");
      memset( cliInputBuffer, 0x00, CLI_INPUT_BUFFER_SIZE );
      ucInputIndex = 0;
      ssUartPutc(cliUart, '>');
    }
      
    else if( ( c == '\b' ) || ( c == ASCII_DEL ) )
    {
      /* Backspace was pressed.  Erase the last character in the string - if any. */
      if(updateInputFromHistory)
      {
        /* Line from history was being edited, use that one as an input line from now. */
        strcpy(cliInputBuffer, cliInputHistory[historyCurrentIndex]);
        ucInputIndex = strlen(cliInputBuffer);
        updateInputFromHistory = pdFALSE;
      }
      if(ucInputIndex > 0)
      {
        ucInputIndex--;
        cliInputBuffer[ucInputIndex] = '\0';
        ssUartPutc(cliUart, c);
      }
    }
    else
    {
      /* A character was entered.
         Check if it is printable.
         If yes, add it to the string entered so far. 
      */
			if( ( c >= ' ' ) && ( c <= '~' ) )
			{
        if(updateInputFromHistory)
        {
          /* Line from history was being edited, use that one as an input line from now. */
          strcpy(cliInputBuffer, cliInputHistory[historyCurrentIndex]);
          ucInputIndex = strlen(cliInputBuffer);
          updateInputFromHistory = pdFALSE;
        }
				if( ucInputIndex < CLI_INPUT_BUFFER_SIZE )
				{
					cliInputBuffer[ucInputIndex] = c;
					ucInputIndex++;
          ssUartPutc(cliUart, c);
				}
			}    
    }
    
  }
}



#if(configGENERATE_RUN_TIME_STATS == 1)
static BaseType_t CliCommonTopCommand( char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
const char * const pcHeader = "  Abs Time      % Time\r\n****************************************\r\n";
BaseType_t xSpacePadding;

  /* Remove compile time warnings about unused parameters, and check the
  write buffer is not NULL.  NOTE - for simplicity, this example assumes the
  write buffer length is adequate, so does not check for buffer overflows. */
  configASSERT(writeBuffer);

  /* Generate a table of task stats. */
  strcpy( writeBuffer, "Task" );
  writeBuffer += strlen( writeBuffer );

  /* Pad the string "task" with however many bytes necessary to make it the
  length of a task name.  Minus three for the null terminator and half the
  number of characters in	"Task" so the column lines up with the centre of
  the heading. */
  for( xSpacePadding = strlen( "Task" ); xSpacePadding < ( configMAX_TASK_NAME_LEN - 3 ); xSpacePadding++ )
  {
    /* Add a space to align columns after the task's name. */
    *writeBuffer = ' ';
    writeBuffer++;

    /* Ensure always terminated. */
    *writeBuffer = 0x00;
  }

  strcpy( writeBuffer, pcHeader );
  vTaskGetRunTimeStats( writeBuffer + strlen( pcHeader ) );

  /* There is no more data to return after this single string, so return
  pdFALSE. */
  return pdFALSE;
}
	
#endif /* configGENERATE_RUN_TIME_STATS */


#ifdef __cplusplus
}
#endif

#if(configGENERATE_RUN_TIME_STATS == 1)
static BaseType_t CliCommonProcCommand( char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
  const char * const pcHeader = "\tStatus\tPrio\tStack\tTask Num\r\n";
  BaseType_t xSpacePadding;

  /* Remove compile time warnings about unused parameters, and check the
  write buffer is not NULL.  NOTE - for simplicity, this example assumes the
  write buffer length is adequate, so does not check for buffer overflows. */
  configASSERT(writeBuffer);

  /* Generate a table of task stats. */
  strcpy( writeBuffer, "Task" );
  writeBuffer += strlen( writeBuffer );

  /* Pad the string "task" with however many bytes necessary to make it the
  length of a task name.  Minus three for the null terminator and half the
  number of characters in	"Task" so the column lines up with the centre of
  the heading. */
  for( xSpacePadding = strlen( "Task" ); xSpacePadding < ( configMAX_TASK_NAME_LEN - 3 ); xSpacePadding++ )
  {
    /* Add a space to align columns after the task's name. */
    *writeBuffer = ' ';
    writeBuffer++;

    /* Ensure always terminated. */
    *writeBuffer = 0x00;
  }

  strcpy( writeBuffer, pcHeader );
  vTaskList( writeBuffer + strlen( pcHeader ) );

  /* There is no more data to return after this single string, so return
  pdFALSE. */
  return pdFALSE;
}
	
#endif /* configGENERATE_RUN_TIME_STATS */

static BaseType_t CliCommonHistoryCommand( char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
  BaseType_t ret = pdTRUE;
  static uint32_t historyIndex = 0;
  
  snprintf(writeBuffer, size - 1, "%s\n\r", cliInputHistory[historyIndex]);
  historyIndex++;
  if((historyIndex == CLI_INPUT_HISTORY) || (cliInputHistory[historyIndex] == 0))
  {
    historyIndex = 0;
    ret = pdFALSE;
  }

  
  return ret;
}
