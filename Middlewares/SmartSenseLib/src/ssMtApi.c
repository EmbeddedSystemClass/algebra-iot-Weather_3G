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

#ifdef __cplusplus
extern "C" {
#endif
#include <string.h>
#include <stdio.h>
   
#include "ssUart.h"
#include "ssSysCom.h"
#include "ssMtApi.h"
  
#include "FreeRTOS.h"
#include "task.h"
  
/*------------------------- MACRO DEFINITIONS --------------------------------*/
#define MTAPI_TX_TASK_STACK_SIZE        240
#define MTAPI_TX_TASK_PRIORITY          4
#define MTAPI_TX_TASK_NAME              "MTAPI_TX"

#define MTAPI_RX_TASK_STACK_SIZE     240
#define MTAPI_RX_TASK_PRIORITY       4
#define MTAPI_RX_TASK_NAME           "MTAPI_RX"
   
#define MTAPI_USART_BUFFER_SIZE       128

#define MTAPI_TX_QUEUE_SIZE            8
#define MTAPI_TX_MSG_RECEIVE_TIMEOUT   portMAX_DELAY
#define MTAPI_ACK_RCV_QUEUE_SIZE       4
#define MTAPI_ACK_RCV_RECEIVE_TIMEOUT  1000
  
#define MTAPI_TIMEOUT                  1000
  
/* MT uart configuration */
#define MTAPI_UART                   USART2
#define MTAPI_UART_BAUDRATE          19200
#define MTAPI_UART_WORDLENGTH        UART_WORDLENGTH_8B
#define MTAPI_UART_STOPBITS          UART_STOPBITS_1  
#define MTAPI_UART_PARITY            UART_PARITY_NONE
#define MTAPI_UART_MODE              UART_MODE_TX_RX
#define MTAPI_UART_FLOWCONTROL       UART_HWCONTROL_NONE

/* MT receiver states */
#define MT_RCV_STATE_SOF  0
#define MT_RCV_STATE_LEN  1
#define MT_RCV_STATE_CMD  2
#define MT_RCV_STATE_FCS  3  
  
  
uint32_t frame_cnt = 0;  
uint32_t sreq_cnt = 0;
uint32_t areq_cnt = 0;
uint32_t srsp_cnt = 0;
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
  
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/
static int32_t mt_uart = -1;

static MtApiSysCbk MtApiSysCbkTable[MT_SUBSYSTEM_MAX] = {NULL};

static ssSysComCpidType MtMsgSendCpid;
static ssSysComCpidType MtAckRcvCpid;

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/
static void MtApiSenderTask(void * argument);
static void MtApiReceiverTask(void * argument);

static uint8_t *MtApiFrameReceive(uint32_t timeout);

void MtApiSendCmdReqMsgHandle(void *cmd);

void MtApiFrameSend(const uint8_t *const cmd);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
void ssMtApiInit(void)
{
  BaseType_t ret;
  ssUartConfigType config;
  
  config.baudrate = MTAPI_UART_BAUDRATE;
  config.FlowControl = MTAPI_UART_FLOWCONTROL;
  config.Mode = MTAPI_UART_MODE;
  config.Parity = MTAPI_UART_PARITY;
  config.StopBits = MTAPI_UART_STOPBITS;
  config.WordLength = MTAPI_UART_WORDLENGTH;
  mt_uart = ssUartOpen(MTAPI_UART, &config, MTAPI_USART_BUFFER_SIZE);
  configASSERT(mt_uart >= 0);
  
  ret = xTaskCreate(MtApiSenderTask,
                    MTAPI_TX_TASK_NAME,
                    MTAPI_TX_TASK_STACK_SIZE, /* Stack depth in words. */
                    NULL, /* We are not using the task parameter. */
                    MTAPI_TX_TASK_PRIORITY, /* This task will run at priority 1. */
                    NULL /* We are not going to use the task handle. */
                    );
  configASSERT(ret == pdPASS);
  
  ret = xTaskCreate(MtApiReceiverTask,
                    MTAPI_RX_TASK_NAME,
                    MTAPI_RX_TASK_STACK_SIZE, 
                    NULL, 
                    MTAPI_RX_TASK_PRIORITY, 
                    NULL 
                    );
  configASSERT(ret == pdPASS);
}

uint32_t crc_err=0;

uint8_t *MtApiFrameReceive(uint32_t timeout)
{
  uint8_t c;
  uint8_t cmdlen = 0;
  uint8_t fcs;
  uint8_t cmdidx = 0;
  uint8_t *cmd = NULL;
  uint32_t done = 0;
  uint32_t state = MT_RCV_STATE_SOF;
  uint32_t i;
  (void)timeout; /* not used */
  
  /* MT command format:
   *          | SOP | Data Length  |   CMD   |   Data   |  FCS  |
   *          |  1  |     1        |    2    |  0-Len   |   1   |
   */
  do
  {
    c = ssUartGetc(mt_uart, portMAX_DELAY);
    
    switch(state)
    {
    case MT_RCV_STATE_SOF:
      if(c == MT_FRAME_SOF)
      {
        state = MT_RCV_STATE_LEN;
      }
      break;
    case MT_RCV_STATE_LEN:
      cmdlen = c + MT_FRAME_CMD_SIZE + MT_FRAME_LEN_SIZE;
      cmd = pvPortMalloc(cmdlen);
      if(cmd != NULL)
      {
        cmdidx = 0;
        cmd[cmdidx++] = c;
        state = MT_RCV_STATE_CMD;
      }
      else
      {
        done = 1;
      }
      break;
    case MT_RCV_STATE_CMD:
      cmd[cmdidx++] = c;
      if(cmdidx == cmdlen)
      {
        state = MT_RCV_STATE_FCS;
      }
      break;
    case MT_RCV_STATE_FCS:
      fcs = 0;
      for(i=0; i< cmdlen; i++)
      {
        fcs ^= cmd[i];
      }
      if((fcs ^ c) != 0)
      {
        /* checksum error; discard received data */
        vPortFree(cmd);
        cmd = NULL;
        crc_err++;
      }
      done = 1;
    }
  } while(!done);
  
  return cmd;
}


uint32_t MtApiRegisterSubsystem(MtApiSubsystemTypeEnum subsystem, MtApiSysCbk callback)
{   
  if(subsystem < MT_SUBSYSTEM_MAX)
  {
    /* @TODO: should we reject or override already registered subsystems? */
    MtApiSysCbkTable[subsystem] = callback;
  }
  
  return 0;
}

void MtApiCmdSend(uint8_t cmdType, uint8_t cmdId, uint8_t dataLen, uint8_t *pData)
{
}


uint32_t sreq_sent=0;
uint32_t areq_sent=0;

uint8_t *MtApiCmdSendX(uint8_t *mtcmd)
{
  void *msg;
  uint8_t *payload;
  uint8_t *mtresp = NULL;
  
  configASSERT(mtcmd != NULL);
  
  msg = ssSysComMsgCreate(MTAPI_SEND_CMD_REQ_MSG_ID, MT_CMD_FRAME_LENGTH_GET(mtcmd), MtMsgSendCpid);
  configASSERT(msg);
   
  payload = (uint8_t *)ssSysComMsgPayloadGet(msg);
  memcpy(payload, mtcmd, MT_CMD_FRAME_LENGTH_GET(mtcmd));
  
  if(MT_CMD_TYPE_GET(mtcmd) == MT_CMD_TYPE_SREQ)
  {
    /* synchronous request, send the command and wait for the response */
    ssSysComCpidType cpid = SS_SYSCOM_CPID_INVALID;
    QueueHandle_t queue = NULL;
    void *reply;
    uint8_t mtsize;
    
    sreq_sent++;
      
    queue = ssSysComQueueCreate(SS_SYSCOM_QUEUE_SIZE_DEFAULT);
    configASSERT(queue);
    cpid = ssSysComUserRegister(SS_SYSCOM_CPID_INVALID, queue);
    configASSERT(cpid != SS_SYSCOM_CPID_INVALID);
    ssSysComMsgSendS(&msg, cpid);
    reply = (MtApiCmdRespType *)ssSysComMsgReceive(cpid, MTAPI_TIMEOUT);
    ssSysComUserDeregister(cpid, pdTRUE);
    
    if(reply)
    {
      mtsize = ssSysComMsgPayloadSizeGet(reply);
      mtresp = pvPortMalloc(mtsize);
      configASSERT(mtresp);
      memcpy(mtresp, ssSysComMsgPayloadGet(reply), mtsize);
      ssSysComMsgDestroy(&reply);
    }
  }
  else
  {
    /* asynchronous request, don't wait for response */
    areq_sent++;
    ssSysComMsgSend(&msg);
  }
  
  return mtresp;
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

void MtApiSenderTask(void * argument)
{
  /* register sender cpids */
  MtMsgSendCpid = ssSysComUserRegister(SS_SYSCOM_CPID_INVALID, ssSysComQueueCreate(MTAPI_TX_QUEUE_SIZE));
  configASSERT(MtMsgSendCpid != SS_SYSCOM_CPID_INVALID);
  MtAckRcvCpid = ssSysComUserRegister(SS_SYSCOM_CPID_INVALID, ssSysComQueueCreate(MTAPI_ACK_RCV_QUEUE_SIZE));
  configASSERT(MtAckRcvCpid != SS_SYSCOM_CPID_INVALID);
  
  while (1)
  {   
    void *msg = ssSysComMsgReceive(MtMsgSendCpid, MTAPI_TX_MSG_RECEIVE_TIMEOUT);
    if(msg != NULL)
    {
      ssSysComMsgIdType msgid = ssSysComMsgIdGet(msg);
             
      if(msgid == MTAPI_SEND_CMD_REQ_MSG_ID)
      {
        MtApiSendCmdReqMsgHandle(msg);
      }      
      
      ssSysComMsgDestroy(&msg);
    }
  }
}

uint32_t mtApiWaitSreq = pdFALSE;

void MtApiSendCmdReqMsgHandle(void *msg)
{
  uint8_t *mtMsg = (uint8_t *)ssSysComMsgPayloadGet(msg);
  
  MtApiFrameSend(mtMsg);
  
  if(MT_CMD_TYPE_GET(mtMsg) == MT_CMD_TYPE_SREQ)
  {
    /* wait for the response */ 
    mtApiWaitSreq = pdTRUE;
    void *rsp = ssSysComMsgReceive(MtAckRcvCpid, MTAPI_ACK_RCV_RECEIVE_TIMEOUT);
    mtApiWaitSreq = pdFALSE;
    if(rsp != NULL)
    {
      if(ssSysComMsgIdGet(rsp) == MTAPI_SEND_CMD_RESP_MSG_ID)
      {
        ssSysComMsgForward(&rsp, ssSysComMsgSenderGet(msg));
      }
      else
      {
        /* wrong message */
        /* @TODO: should we exit or continue to wait for srsp? */ 
        /* delete it in any case */
        ssSysComMsgDestroy(&rsp);
      }
    }
    else
    {
      /* no response message */
    }
  }                                  
}


void MtApiFrameSend(const uint8_t *const cmd)
{
  uint8_t fcs = 0;
  uint8_t i;

  ssUartPutc(mt_uart, MT_FRAME_SOF);
  ssUartPutc(mt_uart, cmd[MT_CMD_LEN_POS]);
  fcs ^= cmd[MT_CMD_LEN_POS];
  ssUartPutc(mt_uart, cmd[MT_CMD_CMD0_POS]);
  fcs ^= cmd[MT_CMD_CMD0_POS];
  ssUartPutc(mt_uart, cmd[MT_CMD_CMD1_POS]);
  fcs ^= cmd[MT_CMD_CMD1_POS];
  
  for(i=0; i<cmd[MT_CMD_LEN_POS]; i++)
  {
    ssUartPutc(mt_uart, cmd[MT_CMD_DATA_POS+i]);
    fcs ^= cmd[MT_CMD_DATA_POS+i];
  }
  ssUartPutc(mt_uart, fcs);
}

void MtApiReceiverTask(void * argument)
{  
  static uint32_t dropped = 0;
  while (1)
  {
    uint8_t *mtMsg = MtApiFrameReceive(0);
    if(mtMsg != NULL)
    {
      MtApiSysCbk callback;
      uint8_t status = MT_SUCCESS;
      uint8_t size = MT_CMD_DATA_LENGTH_GET(mtMsg);
      uint8_t subsystem = MT_CMD_SUBSYSTEM_GET(mtMsg);
      uint8_t cmdtype = MT_CMD_TYPE_GET(mtMsg);
      uint8_t cmdid = MT_CMD_ID_GET(mtMsg);
      uint8_t *data = MT_CMD_DATA_PTR_GET(mtMsg);
      
      frame_cnt++;
      if(cmdtype == MT_CMD_TYPE_SREQ)
        sreq_cnt++;
      else if(cmdtype == MT_CMD_TYPE_SRSP)
        srsp_cnt++;
      else if(cmdtype == MT_CMD_TYPE_AREQ)
        areq_cnt++;
      
      if(cmdtype == MT_CMD_TYPE_SRSP)
      {
        /* Responses are dispached to sender via transmitter task */
        void *msg = NULL;
        if(mtApiWaitSreq == pdTRUE)
        {
          msg = ssSysComMsgCreate(MTAPI_SEND_CMD_RESP_MSG_ID, MT_CMD_FRAME_LENGTH_GET(mtMsg), MtAckRcvCpid);
          configASSERT(msg);
          memcpy(ssSysComMsgPayloadGet(msg), mtMsg, MT_CMD_FRAME_LENGTH_GET(mtMsg));
          ssSysComMsgSend(&msg);
        }
        else
        {
          dropped++;
        }
      }
      else
      {
        if(subsystem < MT_SUBSYSTEM_MAX)
        {
          /* @TODO: complete mt message should be sent to client */
          callback = MtApiSysCbkTable[subsystem];
          if(callback)
          {
            status = callback(cmdid, size, data);
          }
          else
          {
            status = MT_ERR_SUBSYSTEM;
          }
        }
        else
        {
          status = MT_ERR_SUBSYSTEM;
        }
           
        /* Send error if error and sync request */
        if((status != MT_SUCCESS) && (cmdtype == MT_CMD_TYPE_SREQ))
        {
          /* response message:  | status | cmd0 | cmd1 | */
          uint8_t rsp[MT_FRAME_CMD_HEADER_SIZE*2];
          
          MT_CMD_DATA_LENGTH_SET(rsp, MT_FRAME_CMD_HEADER_SIZE);
          MT_CMD_TYPE_SUBSYSTEM_SET(rsp, MT_CMD_TYPE_SRSP, MT_SUBSYSTEM_RES0);
          MT_CMD_ID_SET(rsp, 0);
          rsp[MT_CMD_DATA_POS + 0] = status;
          rsp[MT_CMD_DATA_POS + 1] = mtMsg[MT_CMD_CMD0_POS];
          rsp[MT_CMD_DATA_POS + 2] = mtMsg[MT_CMD_CMD1_POS];
          
          MtApiCmdSendX(rsp);
        }
      }
            
      vPortFree(mtMsg);
    }
  }
}


#ifdef __cplusplus
}
#endif


 