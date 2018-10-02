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

#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "ssLogging.h"
#include "ssTask.h"
#include "FreeRTOS_CLI.h"
#include "ssCli.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

static const char modemCliCommandHelpString[] =
"Modem subcommands:\n\r"
"- help: prints this message\n\r"
"- info: prints modem information (type, hw/sw version...)\n\r"
"- status: prints modem status\n\r"
"- pwr [on|off]: turns the modem on/off; modem power status if no argument"
"- cmd command: sends a single AT command to the modem\n\r"
"- reset: resets modem\n\r";

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

static BaseType_t ModemCliCommand(char *writeBuffer, size_t size, const char *command, const BaseType_t intr);
static BaseType_t ModemCliCommandHelp(char *writeBuffer, size_t size, const char *command, const BaseType_t intr);
static BaseType_t ModemCliCommandInfo(char *writeBuffer, size_t size, const char *command, const BaseType_t intr);
static BaseType_t ModemCliCommandStatus(char *writeBuffer, size_t size, const char *command, const BaseType_t intr);
static BaseType_t ModemCliCommandPwr(char *writeBuffer, size_t size, const char *command, const BaseType_t intr);
static BaseType_t ModemCliCommandCmd(char *writeBuffer, size_t size, const char *command, const BaseType_t intr);
static BaseType_t ModemCliCommandReset(char *writeBuffer, size_t size, const char *command, const BaseType_t intr);

/*------------------------- PRIVATE VARIABLES (2) ----------------------------*/

static const CLI_Command_Definition_t modemCmdDesc =
{
  "modem",
  "modem: modem related commands.\r\n",
  ModemCliCommand,
  -1
};


/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

void ssModemCliInit(void)
{
  configASSERT(FreeRTOS_CLIRegisterCommand(&modemCmdDesc) == pdPASS);
}


/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

/* CLI commands */
static BaseType_t ModemCliCommand(char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
  int8_t paramCnt;
  BaseType_t status = pdFALSE;

  configASSERT(writeBuffer);

  paramCnt = FreeRTOS_GetNumberOfParameters(command);

  if(paramCnt == 0)
  {
    /* No subcommand */
    strncpy(writeBuffer, cliSubcommandErrStr, size - 1);
    writeBuffer[size - 1] = '\0';
    status = pdFALSE;
  }
  else
  {
    const char *subcommand = NULL;
    BaseType_t subcommandLen;

    subcommand = FreeRTOS_CLIGetParameter(command, 1, &subcommandLen);

    if(strncmp(subcommand, "help", subcommandLen) == 0)
    {
      status = ModemCliCommandHelp(writeBuffer, size, NULL, intr);
    }
    else if(strncmp(subcommand, "info", subcommandLen) == 0)
    {
      status = ModemCliCommandInfo(writeBuffer, size, subcommand, intr);
    }
    else if(strncmp(subcommand, "status", subcommandLen) == 0)
    {
      status = ModemCliCommandStatus(writeBuffer, size, subcommand, intr);
    }
    else if(strncmp(subcommand, "pwr", subcommandLen) == 0)
    {
      status = ModemCliCommandPwr(writeBuffer, size, subcommand, intr);
    }
    else if(strncmp(subcommand, "cmd", subcommandLen) == 0)
    {
      status = ModemCliCommandCmd(writeBuffer, size, subcommand, intr);
    }
    else if(strncmp(subcommand, "reset", subcommandLen) == 0)
    {
      status = ModemCliCommandReset(writeBuffer, size, subcommand, intr);
    }


    else
    {
      snprintf(writeBuffer, size-1, "Unknown subcommand\n\r");
      writeBuffer[size-1] = '\0';
      status = pdFALSE;
    }
  }
  return status;
}

static BaseType_t ModemCliCommandHelp(char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
  return pdFALSE;
}

static BaseType_t ModemCliCommandInfo(char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
  return pdFALSE;
}


static BaseType_t ModemCliCommandStatus(char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
  return pdFALSE;
}

static BaseType_t ModemCliCommandPwr(char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
  return pdFALSE;
}


static BaseType_t ModemCliCommandCmd(char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
  return pdFALSE;
}

static BaseType_t ModemCliCommandReset(char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
  return pdFALSE;
}


#ifdef __cplusplus
}
#endif


 
