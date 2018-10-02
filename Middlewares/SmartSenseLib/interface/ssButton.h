/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_BUTTON_H
#define _SS_BUTTON_H

#include <stdint.h>

#include "ssSysCom.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

#define SS_BUTTON_STATUS_OK   0
#define SS_BUTTON_STATUS_ERR  1
  
#define BUTTON_DAEMON_TASK_CPID     0x05
  
#define BUTTON_PRESS_IND_MSG_ID     0x50
  
#define BUTTON_EVENT_NONE           0
#define BUTTON_EVENT_PRESS          1
#define BUTTON_EVENT_RELEASE        2
#define BUTTON_EVENT_ALL            3
  
#define USER_BUTTON_ID  BSP_BUTTON_0
 
#define SS_BUTTON_COUNT    BSP_BUTTON_COUNT

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

typedef struct
{
  uint8_t buttonId;
  uint8_t event;
} ModeCtrlButtonIndMsgType;
  
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

void ssButtonInit(void);

uint32_t ssButtonRegister(ssSysComCpidType cpid, uint8_t buttonId, uint8_t event);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/


#ifdef __cplusplus
}
#endif

#endif /* _SS_BUTTON_H */

