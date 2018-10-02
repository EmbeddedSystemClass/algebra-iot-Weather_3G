/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_TASK_H
#define _SS_TASK_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

#if defined(configNUM_THREAD_LOCAL_STORAGE_POINTERS)
#define SS_TASK_TLS_SIZE configNUM_THREAD_LOCAL_STORAGE_POINTERS
#define SS_TASK_CPID_TLS_INDEX 0  
#define SS_TASK_USER_TLS_INDEX (SS_TASK_CPID_TLS_INDEX + 1)
#define SS_TASK_TLS_INDEX_LAST (SS_TASK_USER_TLS_INDEX)
#if SS_TASK_TLS_INDEX_LAST >= SS_TASK_TLS_SIZE
  #error "Not enough storage for TLS"
#endif
#endif
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/



/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/


#ifdef __cplusplus
}
#endif

#endif /* _SS_TASK_H */

