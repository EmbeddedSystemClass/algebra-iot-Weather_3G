/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SARA_U270_H
#define _SARA_U270_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

typedef const void * ModemHandle_t;

typedef struct ModemConfiguration_t
{
  const char* simpin;
} ModemConfiguration_t;

typedef enum ModemResult_e
{
  ModemResult_OK = 0,
  ModemResult_Error = 1,
  ModemResult_None = 2,
  ModemResult_NotFound = 3,
} ModemResult_e;

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
ModemHandle_t SARA_U270_Open(ModemConfiguration_t *mdmconfig);

uint32_t SARA_U270_connect(ModemHandle_t modemHandle);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _SARA_U270_H */
 
