/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_CLI_H
#define _SS_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/
extern const char * const cliSubcommandErrStr;  
extern const char * const cliParameterErrStr; 
extern const char * const cliParamValueErrStr; 

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
void ssCliUartConsoleInit(void);
void ssCliUartConsoleStart(void);

void ssCliDebugPrint(char* format, ...);
  
  
#ifdef __cplusplus
}
#endif

#endif /* _SS_CLI_H */
 