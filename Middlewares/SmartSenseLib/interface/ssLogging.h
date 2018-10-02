/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_LOGGING_H
#define _SS_LOGGING_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
 
#define LOGGING_NO_CATEGORY 0
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef enum ESsLoggingLevel
{
  ESsLoggingLevel_NotValid = 0,
  ESsLoggingLevel_Debug = 1,
  ESsLoggingLevel_Info = 2,
  ESsLoggingLevel_Warning = 3,
  ESsLoggingLevel_Error = 4,
  ESsLoggingLevel_NoPrints = 5,
} ESsLoggingLevel;

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

void ssLoggingInit(void);

void ssLoggingPrint(const ESsLoggingLevel level,
                    const uint32_t category, 
                    const char* unformattedStringPtr,
                    ...);

void ssLoggingPrintRawStr(const ESsLoggingLevel level,
                          const uint32_t category,
                          const char* string, 
                          int len,
                          const char* unformattedStringPtr,
                          ...);
  
/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _SS_LOGGING_H */
 
