/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_SYSCOM_H
#define _SS_SYSCOM_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

#define SS_SYSCOM_CPID_INVALID      0
#define SS_SYSCOM_CPID_STAT_START   1
#define SS_SYSCOM_CPID_STAT_CNT     64
#define SS_SYSCOM_CPID_DYN_START    (SS_SYSCOM_CPID_STAT_START + SS_SYSCOM_CPID_STAT_CNT)
#define SS_SYSCOM_CPID_DYN_CNT      32
#define SS_SYSCOM_CPID_CNT          (SS_SYSCOM_CPID_DYN_START + SS_SYSCOM_CPID_DYN_CNT)

#define SS_SYSCOM_QUEUE_SIZE_DEFAULT  4
#define SS_SYSCOM_QUEUE_ENTRY_SIZE    (sizeof(void *))
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef uint16_t ssSysComQueueSizeType;
typedef uint16_t ssSysComMsgIdType;
typedef uint8_t ssSysComCpidType;
typedef uint16_t ssSysComMsgSizeType;
typedef uint8_t ssSysComMsgPayloadType;

typedef struct ssSysComMsgHeaderType
{
  ssSysComCpidType sender;
  ssSysComCpidType receiver;
  ssSysComCpidType owner;
  uint8_t flags;
  ssSysComMsgSizeType size;
  ssSysComMsgIdType msgid;
} ssSysComMsgHeaderType;

typedef struct ssSysComMsgType
{
  ssSysComMsgHeaderType header;
  ssSysComMsgPayloadType payload[0];
} ssSysComMsgType;

typedef enum ssSysComMtmEnum
{
  ssSysComMtm_Reliable,   /**< Reliable transfer mode. */
  ssSysComMtm_Basic       /**< Basic (non-reliable) transfer mode. */
} ssSysComMtmEnum;

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

void ssSysComInit(void);

void *ssSysComQueueCreate(ssSysComQueueSizeType size);
void ssSysComQueueDestroy(void **queue);

ssSysComCpidType ssSysComUserRegister(ssSysComCpidType cpid, QueueHandle_t queue);
void ssSysComUserDeregister(ssSysComCpidType cpid, uint32_t deleteQueue);

void* ssSysComMsgCreate(ssSysComMsgIdType msgid, ssSysComMsgSizeType size, ssSysComCpidType target);
void* ssSysComMsgCreateReply(ssSysComMsgIdType msgid, ssSysComMsgSizeType size, void *repliedMsg);

void ssSysComMsgDestroy(void **user_msg);

void* ssSysComMsgReceive(ssSysComCpidType cpid, uint32_t timeout);
void* ssSysComMsgReceiveSelective(ssSysComCpidType cpid, uint32_t timeout, ssSysComMsgIdType *msgid);

void ssSysComMsgSendS(void **user_msg, ssSysComCpidType sender);
void ssSysComMsgSend(void **user_msg);
void ssSysComMsgForward(void **user_msg, ssSysComCpidType target);

ssSysComMsgIdType ssSysComMsgIdGet(void *user_msg);
void ssSysComMsgIdSet(void *user_msg, ssSysComMsgIdType msgid);
void *ssSysComMsgPayloadGet(void *user_msg);
ssSysComMsgSizeType ssSysComMsgPayloadSizeGet(void *user_msg);
ssSysComCpidType ssSysComMsgSenderGet(void *user_msg);
void ssSysComMsgSenderSet(void *user_msg, ssSysComCpidType sender);
ssSysComCpidType ssSysComMsgReceiverGet(void *user_msg);
void ssSysComMsgReceiverSet(void *user_msg, ssSysComCpidType receiver);
ssSysComCpidType ssSysComMsgOwnerGet(void *user_msg);

/* set message transfer mode */
void ssSysComMsgSetMtm(const void *user_msg, const ssSysComMtmEnum requestedTransferMode);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/


#ifdef __cplusplus
}
#endif

#endif /* _SS_SYSCOM_H */

