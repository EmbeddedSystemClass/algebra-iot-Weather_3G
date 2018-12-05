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

#include <string.h>
#include <stdio.h>
  
#include "FreeRTOS.h"   
#include "FreeRTOS_CLI.h"
#include "task.h"
  
#include "ssCli.h"
//#include "ssSupervision.h"

#include "Version.h"
 
#include "bsp.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/
static BaseType_t CliCommonCmdVersion(char *writeBuffer, size_t size, const char *command, const BaseType_t intr);
static BaseType_t CliCommonCmdReboot(char *writeBuffer, size_t size, const char *command, const BaseType_t intr);
static BaseType_t CliCommonCmdSerial(char *writeBuffer, size_t size, const char *command, const BaseType_t intr);

static const CLI_Command_Definition_t cmdVersionDesc =
{
  "version",
  "version: prints host sw/hw version.\r\n",
  CliCommonCmdVersion,
  0
};

static const CLI_Command_Definition_t cmdRebootDesc =
{
  "reboot",
  "reboot [host/board]: reboot host or board.\r\n",
  CliCommonCmdReboot,
  1
};

static const CLI_Command_Definition_t cmdSerialDesc =
{
  "serial",
  "serial: prints device serial number.\r\n",
  CliCommonCmdSerial,
  0
};

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
void CliCommonCmdsInit(void)
{
  configASSERT(FreeRTOS_CLIRegisterCommand(&cmdVersionDesc) == pdPASS);
  configASSERT(FreeRTOS_CLIRegisterCommand(&cmdRebootDesc) == pdPASS);
  configASSERT(FreeRTOS_CLIRegisterCommand(&cmdSerialDesc) == pdPASS);
}
/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

static BaseType_t CliCommonCmdVersion(char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
  snprintf(writeBuffer, size-1, "%s; v%hhu.%hhu.%hhu.%hhu; built: %s by %s\n\r", 
           board_name, 
           build_version[0], build_version[1], build_version[2], build_version[3],
           build_date,
           build_author);
  return pdFALSE;
}

static BaseType_t CliCommonCmdReboot(char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
  const char *param;
  BaseType_t paramLen;
  
  param = FreeRTOS_CLIGetParameter(command, 1, &paramLen);
  if(strncmp(param, "host", paramLen) == 0)
  {
    snprintf(writeBuffer, size-1, "Host is going down...\n\r");
    vTaskDelay(1000/portTICK_RATE_MS);
    NVIC_SystemReset();
  }
  else if(strncmp(param, "board", paramLen) == 0)
  {
    snprintf(writeBuffer, size-1, "Board is going down...\n\r");
    vTaskDelay(1000/portTICK_RATE_MS);
//    ssSupervisionReboot();
  }
  else
  {
    snprintf(writeBuffer, size-1, "Invalid parameter\n\r");
  }
  
  return pdFALSE;
}


static BaseType_t CliCommonCmdSerial(char *writeBuffer, size_t size, const char *command, const BaseType_t intr)
{
  snprintf(writeBuffer, size-1, "OCA1000001\n\r");
  return pdFALSE;
}

#ifdef __cplusplus
}
#endif


 