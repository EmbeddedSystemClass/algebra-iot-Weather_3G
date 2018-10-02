/**
 * @file     
 * @brief    Simple Finite State Machine implementation 
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_FSM_H
#define _SS_FSM_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

#define FSM_EVENT_INVALID     0
#define FSM_EVENT_START       1
#define FSM_EVENT_ENTRY       2
#define FSM_EVENT_EXIT        3
#define FSM_EVENT_USER_START  4
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef uint32_t ssFsmEventType;
typedef struct ssFsmStateType ssFsmStateType;
typedef struct ssFsmType ssFsmType;
typedef void (*ssFsmEventHandler)(ssFsmType*, ssFsmEventType);


struct ssFsmStateType
{
  ssFsmEventHandler handler;
  char const *name;
};

struct ssFsmType
{
  ssFsmStateType *current;
  ssFsmStateType *next;
  char const *name;
};
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
void ssFsmCtor(ssFsmType *me, char const *name); 
void ssFsmStart(ssFsmType *me, ssFsmStateType *start_state); 
void ssFsmOnEvent(ssFsmType *me, ssFsmEventType event); /* FSM engine */
void ssFsmStateCtor(ssFsmStateType *me, char const *name, ssFsmEventHandler handler);

ssFsmStateType *ssFsmCurrentStateGet(ssFsmType *me);
void ssFsmStateStart(ssFsmType *me, ssFsmStateType *target);
void ssFsmStateTrans(ssFsmType *me, ssFsmStateType *target);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _TEMPLATE_H */ 