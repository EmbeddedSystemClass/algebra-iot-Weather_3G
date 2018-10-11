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
#include "bsp.h"
#include "ssSysCom.h"
#include "ssSupervision.h"
  
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "ssLogging.h"


/*------------------------- MACRO DEFINITIONS --------------------------------*/
  
#define SUPERVISION_DAEMON_NAME       "WDTask"
#define SUPERVISION_DAEMON_STACK_SIZE 240
#define SUPERVISION_DAEMON_PRIORITY   10
  
#define SUPERVISION_WD_FEED_TIMER_ID  0
#define SUPERVISION_WD_FEED_PERIOD    250
#define SUPERVISION_WD_FEED_REQ_MSG_ID  0x0600
  
#define SUPERVISION_MSG_RECEIVE_TIMEOUT (SUPERVISION_WD_FEED_PERIOD*2)
  
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/
TimerHandle_t wdFeedTimer;

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/
void SupervisionDaemon(void *argument);
void SupervisionTimerCallback(TimerHandle_t xTimer);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
void ssSupervisionInit(void)
{
  BaseType_t ret;
 
  ret = xTaskCreate(SupervisionDaemon,
                    SUPERVISION_DAEMON_NAME,
                    SUPERVISION_DAEMON_STACK_SIZE, /* Stack depth in words. */
                    NULL, /* We are not using the task parameter. */
                    SUPERVISION_DAEMON_PRIORITY, /* This task will run at priority 1. */
                    NULL
                    );
  configASSERT(ret == pdPASS);
}

void ssSupervisionReboot(void)
{
  /* stop the feeder */
  configASSERT(xTimerStop(wdFeedTimer, MILLISECONDS_TO_OS_TICKS(100)));
}
  
/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

void SupervisionDaemon(void *argument)
{
  void *msg;
  uint32_t feedskip = 0;
  
  configASSERT(ssSysComUserRegister(SS_SUPERVISION_TASK_CPID, ssSysComQueueCreate(SS_SYSCOM_QUEUE_SIZE_DEFAULT)) == SS_SUPERVISION_TASK_CPID);
  
  wdFeedTimer = xTimerCreate("WdFeedTmr",
                                MILLISECONDS_TO_OS_TICKS(SUPERVISION_WD_FEED_PERIOD),
                                pdTRUE,
                                (void *)SUPERVISION_WD_FEED_TIMER_ID,
                                SupervisionTimerCallback);
  configASSERT(xTimerStart(wdFeedTimer, 0) == pdPASS);
   
  while(1)
  {
    msg = ssSysComMsgReceive(SS_SUPERVISION_TASK_CPID, SUPERVISION_MSG_RECEIVE_TIMEOUT);
    if(msg != NULL)
    {
      ssSysComMsgIdType msgid = ssSysComMsgIdGet(msg);
      if(msgid == SUPERVISION_WD_FEED_REQ_MSG_ID)
      {
        BSP_WD_Feed(BSP_WD_EXTERNAL);
      }
      
      ssSysComMsgDestroy(&msg);
    }
    else
    {
      feedskip++;
    }
  }
}


void SupervisionTimerCallback(TimerHandle_t xTimer)
{
  void *msg;
  
  msg = ssSysComMsgCreate(SUPERVISION_WD_FEED_REQ_MSG_ID, 0, SS_SUPERVISION_TASK_CPID); 
  configASSERT(msg);

  ssSysComMsgSend(&msg);
}


void vApplicationStackOverflowHook(TaskHandle_t xTask,  signed char *pcTaskName)
{
  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Task %s: stack overflow with name %s",
                       &xTask, pcTaskName);
  while(1);
}



 
