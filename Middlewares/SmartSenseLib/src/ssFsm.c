/**
 * @file     
 * @brief    Simple Finite State Machine implementation
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
#include <stdint.h>
#include "ssFsm.h"
  
#include "FreeRTOS.h"
#include "task.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/
  
/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

static void ssFsmStateOnEvent(ssFsmType *me, ssFsmStateType *start_state, ssFsmEventType event);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
    
void ssFsmCtor(ssFsmType *me, char const *name) 
{
   me->name = name;
   me->current = NULL;
   me->next = NULL;
}
    
void ssFsmStateCtor(ssFsmStateType *me, char const *name, ssFsmEventHandler handler)
{
  me->handler = handler;
  me->name = name;
}

void ssFsmStart(ssFsmType *me, ssFsmStateType *start_state)
{
  me->current = start_state;
  me->next = NULL;
  ssFsmOnEvent(me, FSM_EVENT_START);
}

void ssFsmOnEvent(ssFsmType *me, ssFsmEventType event)
{
  ssFsmStateOnEvent(me, me->current, event);
  while((me->current != me->next) && me->next != NULL)
  {
    /* State transition taken */
    me->current = me->next;
    me->next = NULL;
    ssFsmStateOnEvent(me, me->current, FSM_EVENT_ENTRY);
  }
}

ssFsmStateType *ssFsmCurrentStateGet(ssFsmType *me)
{
  return me->current;
}

void ssFsmStateStart(ssFsmType *me, ssFsmStateType *target)
{
  me->next = target;
}

void ssFsmStateTrans(ssFsmType *me, ssFsmStateType *target)
{
  /* exit current state */
  configASSERT(me->next == NULL);
  ssFsmStateOnEvent(me, me->current, FSM_EVENT_EXIT);
  me->next = target;
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

static void ssFsmStateOnEvent(ssFsmType *me, ssFsmStateType *state, ssFsmEventType event)
{
  state->handler(me, event);
}

#ifdef __cplusplus
}
#endif