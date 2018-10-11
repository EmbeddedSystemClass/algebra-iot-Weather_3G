/**
 * @file     ssSysCom.c
 * @brief    System comunication module implementation. 
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

/*------------------------- INCLUDED FILES -----------------------------------*/
#include "stdio.h"
#include "stdbool.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"

#include "ssSysCom.h"
#include "ssTask.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

#define SYSCOM_QUEUE_NAME_SIZE  8
  
#define SS_SYSCOM_ALLOC pvPortMalloc
#define SS_SYSCOM_FREE  vPortFree

#define SS_SYSCOM_MSG_SIZE_MAX 256U
#define SS_SYSCOM_MSG_PAYLOAD_SIZE_MAX (SS_SYSCOM_MSG_SIZE_MAX - sizeof(ssSysComMsgHeaderType) - sizeof(ssSysComMsgIdType))
#define SS_SYSCOM_MSG_ACTUAL_SIZE(payloadSize)  (payloadSize + sizeof(ssSysComMsgType))

#define SS_SYSCOM_USER_MSG_PTR_GET(msg)             ((void *)((ssSysComMsgType *)msg)->payload)
#define SS_SYSCOM_USER_MSG_PREAMBLE_GET(user_msg)   ((ssSysComMsgType *)((uint8_t *)user_msg - sizeof(ssSysComMsgHeaderType)))
  
#define UF_DEFAULT                0
#define UF_BEST_EFFORT_TRANSPORT  0x01
  
#define SS_SYSCOM_QUEUE_NULL  (-1)
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/ 
  
typedef struct ssSysComQueueEntryType
{
  int16_t next;
  int16_t prev;
  ssSysComMsgType *message;
} ssSysComQueueEntryType;   
  
typedef struct ssSysComQueueType
{
  char name[SYSCOM_QUEUE_NAME_SIZE];
  QueueHandle_t osqueue;
  ssSysComQueueEntryType *rcvqueue;
  //SemaphoreHandle_t queue_mutex;
  ssSysComQueueSizeType queue_size;
  int16_t head;
  int16_t tail;
  int16_t cnt;
} ssSysComQueueType;

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/


static ssSysComQueueType  *m_SysComQueueTable[SS_SYSCOM_CPID_CNT] = {NULL};
static SemaphoreHandle_t  m_SysComCpidMutex;

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

void ssSysComInit(void)
{
  uint32_t i;
  
  for(i=0; i<SS_SYSCOM_CPID_CNT; i++)
  {
    m_SysComQueueTable[i] = NULL;
  }
  
  m_SysComCpidMutex = xSemaphoreCreateMutex();
  configASSERT(m_SysComCpidMutex);
}

void *ssSysComQueueCreate(ssSysComQueueSizeType size)
{
  ssSysComQueueType *queue = NULL;
  if(size == 0)
  {
    size = SS_SYSCOM_QUEUE_SIZE_DEFAULT;
  }
  
  queue = pvPortMalloc(sizeof(ssSysComQueueType));
  configASSERT(queue);
  
  queue->osqueue = xQueueCreate(size, SS_SYSCOM_QUEUE_ENTRY_SIZE);
  configASSERT(queue->osqueue);
  //queue->queue_mutex = xSemaphoreCreateMutex();
  queue->rcvqueue = (ssSysComQueueEntryType *)pvPortMalloc(sizeof(ssSysComQueueEntryType)*size);
  configASSERT(queue->rcvqueue);
  for(int i=0; i<size; i++)
  {
    queue->rcvqueue[i].next = SS_SYSCOM_QUEUE_NULL;
    queue->rcvqueue[i].prev = SS_SYSCOM_QUEUE_NULL;
    queue->rcvqueue[i].message = NULL;
  }
  queue->queue_size = size;
  queue->head = SS_SYSCOM_QUEUE_NULL;
  queue->tail = SS_SYSCOM_QUEUE_NULL;
  queue->cnt = 0;
  
  return queue;
}

void ssSysComQueueDestroy(void **queue_ptr)
{
  if(queue_ptr != NULL)
  {
    ssSysComQueueType *queue = *queue_ptr;
    if(queue != NULL)
    {
      void *msg;
      /* save the pointer to semaphore, we shall release it after the queue is deleted */
      //SemaphoreHandle_t sem_ptr = queue->queue_mutex;
      
      //xSemaphoreTake(sem_ptr, portMAX_DELAY);
     
      /* clean up os queue */
      while(xQueueReceive(queue->osqueue, &msg, 0) == pdPASS)
      {
        SS_SYSCOM_FREE(msg);
      }
      vQueueDelete(queue->osqueue);
      
      /* clean up task rcv queue */
      while(queue->head != SS_SYSCOM_QUEUE_NULL)
      {
        ssSysComQueueEntryType rcvmsg = queue->rcvqueue[queue->head];
        SS_SYSCOM_FREE(&(rcvmsg.message));
        queue->head = rcvmsg.next;
        queue->cnt--;
      }
      
      vPortFree(queue->rcvqueue);
      
      vPortFree(queue);
      *queue_ptr = NULL;
      
      /* destroy the semaphore now when queue is deleted */
      //xSemaphoreGive(sem_ptr);
      //vSemaphoreDelete(sem_ptr);
    }
    
  }
}

ssSysComCpidType ssSysComUserRegister(ssSysComCpidType cpid, void *queue)
{
  uint32_t i;
   
  configASSERT(cpid <= SS_SYSCOM_CPID_CNT);
  configASSERT(queue);

  xSemaphoreTake(m_SysComCpidMutex, portMAX_DELAY);
  
  if(cpid == SS_SYSCOM_CPID_INVALID)
  {
    /* dynamic cpid: find first available */
    for(i=SS_SYSCOM_CPID_DYN_START; i<SS_SYSCOM_CPID_CNT; i++)
    {
      if(m_SysComQueueTable[i] == NULL)
      {
        cpid = i;
        break;
      }
    }
  }
  
  /* @TODO: should we allow CPID hijack? */
  if(cpid != SS_SYSCOM_CPID_INVALID)
  {
    m_SysComQueueTable[cpid] = queue;
    snprintf(((ssSysComQueueType *)queue)->name, SYSCOM_QUEUE_NAME_SIZE, "SYSQ%d", cpid);
    vQueueAddToRegistry(((ssSysComQueueType *)queue)->osqueue, ((ssSysComQueueType *)queue)->name);
  }
  xSemaphoreGive(m_SysComCpidMutex);
 
  return cpid;
}


void ssSysComUserDeregister(ssSysComCpidType cpid, uint32_t deleteQueue)
{
  QueueHandle_t queue = NULL;
  
  configASSERT(cpid <= SS_SYSCOM_CPID_CNT);
  
  xSemaphoreTake(m_SysComCpidMutex, portMAX_DELAY);
  queue = m_SysComQueueTable[cpid];
  if(queue)
  {
    vQueueUnregisterQueue(queue);
  }
  m_SysComQueueTable[cpid] = NULL;
  xSemaphoreGive(m_SysComCpidMutex);
  
  if(deleteQueue)
  {
    ssSysComQueueDestroy(&queue);
  }
}

void* ssSysComMsgCreate(ssSysComMsgIdType msgid, ssSysComMsgSizeType size, ssSysComCpidType target)
{
  ssSysComMsgType *msg; 
  ssSysComMsgSizeType msgsize;

  /* check parameters */
  configASSERT(size <= SS_SYSCOM_MSG_PAYLOAD_SIZE_MAX);
   
  msgsize = SS_SYSCOM_MSG_ACTUAL_SIZE(size);
  msg = (ssSysComMsgType *)SS_SYSCOM_ALLOC(msgsize);
  configASSERT(msg);
  msg->header.owner = SS_SYSCOM_CPID_INVALID; /* @TODO: not used at the moment */
  msg->header.sender = SS_SYSCOM_CPID_INVALID;
  msg->header.receiver = target;
  msg->header.flags = UF_DEFAULT;
  msg->header.size = size;
  msg->header.msgid = msgid;

  return (void *)msg->payload;
}

void* ssSysComMsgCreateReply(ssSysComMsgIdType msgid, ssSysComMsgSizeType size, void *user_msg)
{
  ssSysComMsgType *msg;
  ssSysComMsgType *reply;
  
  /* check parameters */
  configASSERT(user_msg != NULL);
  configASSERT(size <= SS_SYSCOM_MSG_PAYLOAD_SIZE_MAX);

  msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(user_msg);
  
  reply = (ssSysComMsgType *)SS_SYSCOM_ALLOC(SS_SYSCOM_MSG_ACTUAL_SIZE(size));
  reply->header.owner = SS_SYSCOM_CPID_INVALID; /* @TODO: not used at the moment */
  reply->header.receiver = msg->header.sender;
  reply->header.sender = msg->header.receiver;
  reply->header.size = size;
  reply->header.msgid = msgid;
  
  return (void *)reply->payload;
}


void ssSysComMsgDestroy(void **user_msg)
{
  ssSysComMsgType *msg;
  //ssSysComCpidType cpid = ssSysComCurrentTaskCpidGet();
  
  /* check parameters */
  configASSERT(user_msg);
  configASSERT(*user_msg);
  
  msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(*user_msg);

  /* make sure task owns the message */
  //configASSERT(cpid == msg->header.owner); /* @TODO: not used at the moment */

  SS_SYSCOM_FREE((void *) msg);
  *user_msg = NULL;
}

void* ssSysComMsgReceive(ssSysComCpidType cpid, uint32_t timeout)
{ 
  return ssSysComMsgReceiveSelective(cpid, timeout, NULL);
}


void* ssSysComMsgReceiveSelective(ssSysComCpidType cpid, uint32_t timeout, ssSysComMsgIdType *filter)
{
  void *user_msg = NULL;
  ssSysComMsgType *msg = NULL;
  ssSysComQueueType *queue;
  
  configASSERT(cpid != SS_SYSCOM_CPID_INVALID);
  
  queue = m_SysComQueueTable[cpid];
    
  /* check first if there are old messages in rcv queue that satisfy the filter */
  if(queue->head != SS_SYSCOM_QUEUE_NULL)
  {
    int16_t msgslot = queue->head;
    
    while((msgslot != SS_SYSCOM_QUEUE_NULL) && (msg == NULL))
    {
      if(filter)
      {
        msg = NULL;
        for(int i=0; filter[i]!=0; i++)
        {
          if(queue->rcvqueue[msgslot].message->header.msgid == filter[i])
          {
            msg = queue->rcvqueue[msgslot].message;
            break;
          }
        }
      }
      else
      {
        msg = queue->rcvqueue[msgslot].message;
      }
           
      if(msg != NULL)
      {
        /* message found, remove it from the queue */      
        queue->rcvqueue[queue->rcvqueue[msgslot].prev].next = queue->rcvqueue[msgslot].next;
        queue->rcvqueue[queue->rcvqueue[msgslot].next].prev = queue->rcvqueue[msgslot].prev;
        
        if(queue->head == msgslot)
        {
          queue->head = queue->rcvqueue[msgslot].next;
        }     
        
        if(queue->tail == msgslot)
        {
          queue->tail = queue->rcvqueue[msgslot].prev;
        }
        
        queue->rcvqueue[msgslot].message = NULL;
        queue->rcvqueue[msgslot].prev = SS_SYSCOM_QUEUE_NULL;
        queue->rcvqueue[msgslot].next = SS_SYSCOM_QUEUE_NULL;
        queue->cnt--;
      }
      else
      {
        /* keep on looking */
        msgslot =  queue->rcvqueue[msgslot].next;
      }
    }
  }
           
  if(msg == NULL)
  {
    /* nothing so far, wait for a new message */
    if(xQueueReceive(queue->osqueue, &msg, MILLISECONDS_TO_OS_TICKS(timeout)) == pdPASS)
    {
      if(msg)
      {
        if(filter != NULL)
        {
          bool filtered = true;
          /* check if in the filter, if not put it to the receive queue (list) and wait for more */
          for(int i=0; filter[i] != 0; i++)
          {
            if(msg->header.msgid == filter[i])
            {
              filtered = false;
              break;
            }
          }
          if(filtered)
          {
            int16_t msgslot = SS_SYSCOM_QUEUE_NULL;
            
            /* allocate free slot */
            for(int i=0; i<queue->queue_size; i++)
            {
              if(queue->rcvqueue[i].message == NULL)
              {
                msgslot = i;
                break;
              }
            }
            /* make sure we found one */     
            configASSERT(msgslot != SS_SYSCOM_QUEUE_NULL);

            queue->rcvqueue[msgslot].next = SS_SYSCOM_QUEUE_NULL;
            queue->rcvqueue[msgslot].prev = SS_SYSCOM_QUEUE_NULL;
            queue->rcvqueue[msgslot].message = msg;
            
            if(queue->head == SS_SYSCOM_QUEUE_NULL)
            {
              queue->head = msgslot;
              queue->tail = msgslot;  
            }
            else
            {
              queue->rcvqueue[queue->tail].next = msgslot;
              queue->rcvqueue[msgslot].prev = queue->tail;
              queue->tail = msgslot;
            }
            queue->cnt++;
            msg = NULL;
          }
        }
      }
    }
  }
  
  if(msg)
  {
    //msg->header.owner = cpid; /* @TODO: not used at the moment */
    user_msg = SS_SYSCOM_USER_MSG_PTR_GET(msg);
  }
  
  return user_msg;
}

void ssSysComMsgSend(void **user_msg)
{
  BaseType_t ret;
  ssSysComMsgType *msg;
  BaseType_t reliable;
  QueueHandle_t rcvQueue;
  
  /* Check parameters. */
  configASSERT(user_msg);
  configASSERT(*user_msg);
  //configASSERT(cpid != SS_SYSCOM_CPID_INVALID);
  msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(*user_msg);
  
  /* make sure task owns the message */
  //configASSERT(cpid == header->owner);
  
  msg->header.owner = SS_SYSCOM_CPID_INVALID;
  reliable = (msg->header.flags & UF_BEST_EFFORT_TRANSPORT) == 0;
  rcvQueue = m_SysComQueueTable[msg->header.receiver]->osqueue ;
  
  if(rcvQueue != NULL)
  {
    ret = xQueueSend(rcvQueue, &msg, 0);
    if(!ret)
    {
      /* Failed to send message, probably receiver queue is full. */
      /* Discard the message. */
      ssSysComMsgDestroy(user_msg);
      /* Raise error in case of reliable transfer mode. */
      if(reliable)
      {  
        configASSERT(pdFALSE);
      }
    }
  }
  else
  {
    /* No receiver queue, receiver is probably not registered. */
    /* Silently discard the message. */
    ssSysComMsgDestroy(user_msg);
  }
  *user_msg = NULL;
}


void ssSysComMsgSendS(void **user_msg, ssSysComCpidType sender)
{
  ssSysComMsgType *msg;

  /* check parameters */
  configASSERT(user_msg);
  configASSERT(*user_msg);
  //configASSERT(cpid != SS_SYSCOM_CPID_INVALID);
  msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(*user_msg);
  
  /* make sure task owns the message */
  //configASSERT(cpid == msg->header.owner); /* @TODO: not used at the moment */
  
  msg->header.sender = sender;
  ssSysComMsgSend(user_msg);
  
  /* no need to NULL the callers variable here, AaSysComSend will take care of that */
}


void ssSysComMsgForward(void **user_msg, ssSysComCpidType target)
{
  ssSysComMsgType *msg;

  /* check parameters */
  configASSERT(user_msg);
  configASSERT(*user_msg);
  //configASSERT(cpid != SS_SYSCOM_CPID_INVALID);
  msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(*user_msg);
  
  /* make sure task owns the message */
  //configASSERT(cpid == msg->header.owner); /* @TODO: not used at the moment */
  
  msg->header.sender = ssSysComMsgReceiverGet(*user_msg);
  msg->header.receiver = target;
  ssSysComMsgSend(user_msg);
  
  /* no need to NULL the callers variable here, AaSysComSend will take care of that */
}


ssSysComMsgIdType ssSysComMsgIdGet(void *user_msg)
{
  ssSysComMsgType *msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(user_msg);
  return msg->header.msgid;
}

void ssSysComMsgIdSet(void *user_msg, ssSysComMsgIdType msgid)
{
  ssSysComMsgType *msg; 
  
  configASSERT(user_msg != NULL)
  msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(user_msg);
  msg->header.msgid = msgid;
}

void *ssSysComMsgPayloadGet(void *msg)
{
  return (void *)msg;
}

ssSysComMsgSizeType ssSysComMsgPayloadSizeGet(void *user_msg)
{
  ssSysComMsgType *msg; 
  
  configASSERT(user_msg != NULL)
  msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(user_msg);
  return msg->header.size;
}

ssSysComCpidType ssSysComMsgSenderGet(void *user_msg)
{
  ssSysComMsgType *msg; 
  
  configASSERT(user_msg != NULL)
  msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(user_msg);
  return msg->header.sender;
}


void ssSysComMsgSenderSet(void *user_msg, ssSysComCpidType sender)
{
  ssSysComMsgType *msg; 
  
  configASSERT(user_msg != NULL)
  msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(user_msg);
  msg->header.sender = sender;
}  


ssSysComCpidType ssSysComMsgReceiverGet(void *user_msg)
{
  ssSysComMsgType *msg; 
  
  configASSERT(user_msg != NULL)
  msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(user_msg);
  return msg->header.receiver;
}


void ssSysComMsgReceiverSet(void *user_msg, ssSysComCpidType receiver)
{
  ssSysComMsgType *msg; 
  
  configASSERT(user_msg != NULL)
  msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(user_msg);
  msg->header.receiver = receiver;
}

ssSysComCpidType ssSysComMsgOwnerGet(void *user_msg)
{
  ssSysComMsgType *msg; 
  
  configASSERT(user_msg != NULL)
  msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(user_msg);
  return msg->header.owner;
}

void ssSysComMsgSetMtm(const void *user_msg, const ssSysComMtmEnum requestedTransferMode)
{
  if(user_msg)
  {
    ssSysComMsgType *msg = SS_SYSCOM_USER_MSG_PREAMBLE_GET(user_msg);
    
    if(requestedTransferMode == ssSysComMtm_Reliable)
    {
      msg->header.flags &= ~UF_BEST_EFFORT_TRANSPORT;
    }
    else if (requestedTransferMode == ssSysComMtm_Basic)
    {
      msg->header.flags |= UF_BEST_EFFORT_TRANSPORT;
    }
  }
}
/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/



