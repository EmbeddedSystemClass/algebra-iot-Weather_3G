/**
 * @file     ssCan.c
 * @brief    CAN bus 
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

/*------------------------- INCLUDED FILES ************************************/
#include "bsp.h"
#include "ssCan.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "string.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef struct
{
  CAN_TypeDef* can;
  QueueHandle_t *rx_queue;
  QueueHandle_t *tx_queue;
  uint32_t rx_dropped;
} ssCanType;
  
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/
static ssCanType *m_can = NULL;
CAN_HandleTypeDef hcan;
static CanRxMsgTypeDef RxMessage;

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
uint32_t ssCanInit(uint32_t can_count)
{
  m_can = pvPortMalloc(sizeof(ssCanType));
  if(m_can == NULL)
  {
    return SS_CAN_ERR;
  }  
    
  m_can[0].can = NULL;
  m_can[0].rx_queue = NULL;
  m_can[0].tx_queue = NULL;
  
  return SS_CAN_OK;
}

uint32_t ssCanOpen(CAN_TypeDef* CAN, uint32_t buffer_size, uint8_t baud_rate)
{  
  CAN_FilterConfTypeDef  CAN_FilterInitStructure;

  
  if (m_can[0].can == CAN1) 
  {
    /* CAN already in use */
    goto ssCanOpen_err_0;
  }

  /* Create Queues for RX and TX messages */
  m_can[0].can = CAN1;
  m_can[0].rx_queue = xQueueCreate(buffer_size, sizeof(CanRxMsgTypeDef));
  vQueueAddToRegistry(m_can[0].rx_queue, "CAN RX"); 
  if(m_can[0].rx_queue == NULL)
  {
    goto ssCanOpen_err_1;
  }
  
  m_can[0].tx_queue = xQueueCreate(buffer_size, sizeof(CanTxMsgTypeDef));
  vQueueAddToRegistry(m_can[0].tx_queue, "CAN TX"); 
  if(m_can[0].tx_queue == NULL)
  {
    goto ssCanOpen_err_2;      
  }
  
  /* Initiate CAN bus controller */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 20;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SJW = CAN_SJW_1TQ;
  hcan.Init.BS1 = CAN_BS1_11TQ;
  hcan.Init.BS2 = CAN_BS2_4TQ;
  hcan.Init.TTCM = DISABLE;
  hcan.Init.ABOM = DISABLE;
  hcan.Init.AWUM = DISABLE;
  hcan.Init.NART = DISABLE;
  hcan.Init.RFLM = DISABLE;
  hcan.Init.TXFP = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    goto ssCanOpen_err_3;
  } 
  
  /* Filter and Mask ID for 11-bit STD in [31:21] bits of FilterId and FilterMaskId  */
  CAN_FilterInitStructure.FilterNumber = 0;
  CAN_FilterInitStructure.FilterMode = CAN_FILTERMODE_IDMASK;
  CAN_FilterInitStructure.FilterScale = CAN_FILTERSCALE_32BIT;
  CAN_FilterInitStructure.FilterIdHigh = 0x0000;
  CAN_FilterInitStructure.FilterIdLow = 0x0000;
  CAN_FilterInitStructure.FilterMaskIdHigh = 0x0000;
  CAN_FilterInitStructure.FilterMaskIdLow = 0x0000;
  CAN_FilterInitStructure.FilterFIFOAssignment = CAN_FIFO0;
  CAN_FilterInitStructure.FilterActivation = ENABLE;
  CAN_FilterInitStructure.BankNumber = 0;
  
  HAL_CAN_ConfigFilter(&hcan, &CAN_FilterInitStructure);
  
  /* Start interrupts on FIFO0 */
  RxMessage.IDE = CAN_ID_STD;
  hcan.pRxMsg = &RxMessage; 
  
  HAL_CAN_Receive_IT(&hcan,CAN_FIFO0);

  return SS_CAN_OK;
  
ssCanOpen_err_3:  
  vQueueDelete(m_can[0].tx_queue);
ssCanOpen_err_2:
  vQueueDelete(m_can[0].rx_queue);
ssCanOpen_err_1:
  m_can[0].can = NULL;
  m_can[0].rx_queue = NULL;
  m_can[0].tx_queue = NULL;
ssCanOpen_err_0: 
  return SS_CAN_ERR;
}

uint32_t ssCanTransmit(uint32_t StdId, uint32_t DLC, uint8_t *Data)
{
  CanTxMsgTypeDef canTxBuff;
  
  canTxBuff.RTR = CAN_RTR_DATA;
  canTxBuff.IDE = CAN_ID_STD;
  canTxBuff.StdId = StdId;
  canTxBuff.DLC = DLC;
  for (uint8_t i = 0; i < DLC; i++)
  {
    canTxBuff.Data[i] = Data[i];
  }  
  
  if(xQueueSend(m_can[0].tx_queue, &canTxBuff, 1000 / portTICK_RATE_MS) == errQUEUE_FULL)
  {
    return SS_CAN_ERR;
  }
  
  return SS_CAN_OK;

}

uint32_t ssCanReceiveQueue( CanRxMsgTypeDef *canRxBuff )
{
  uint32_t canRxStatus = SS_CAN_RX_EMPTY;  

  /* Receive Queue for RX */
  if(xQueueReceive(m_can[0].rx_queue, canRxBuff,  0) != pdPASS)
  {
    canRxStatus = SS_CAN_RX_EMPTY;
  }
  else
  {
    canRxStatus = SS_CAN_RX_NEW;
  }
  
  return canRxStatus;
  
}

uint32_t ssCanTransmitQueue(void)
{
  CanTxMsgTypeDef canTxBuff;  
  HAL_StatusTypeDef ret;
  
  /* Receive Queue for TX */
  if(xQueueReceive(m_can[0].tx_queue, &canTxBuff,  0) == pdPASS)
  {
    hcan.pTxMsg = &canTxBuff;
    ret = HAL_CAN_Transmit_IT(&hcan);
    return ret;
  }
  else
  {
    return HAL_OK;
  }
}

uint32_t ssCanFloat2Bytes(uint8_t* bytes_temp, float float_variable)
{ 
  union {
    float f;
    unsigned char c[4];
  } u;
  
  u.f = float_variable;
  memcpy(bytes_temp, u.c, 4);
  
  return 0;
}

uint32_t ssCanFilterSetup(CAN_FilterConfTypeDef  CAN_FilterInitStructure)
{  
  HAL_CAN_ConfigFilter(&hcan, &CAN_FilterInitStructure);  
  return 0;  
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

void HAL_CAN_RxCpltCallback(CAN_HandleTypeDef* hcan)
{
  BaseType_t pxHigherPriorityTaskWoken = pdFALSE;
  
  HAL_CAN_Receive_IT(hcan, CAN_FIFO0);  

  if(xQueueSendFromISR(m_can[0].rx_queue, (void *)hcan->pRxMsg, &pxHigherPriorityTaskWoken) == errQUEUE_FULL)
  {
    m_can[0].rx_dropped++;
  }

}

void HAL_CAN_TxCpltCallback(CAN_HandleTypeDef* hcan)
{
  
}

/**
* @brief This function handles USB high priority or CAN TX interrupts.
*/
void CAN1_TX_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan);
}

/**
* @brief This function handles USB low priority or CAN RX0 interrupts.
*/
void CAN1_RX0_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan);
}

/**
* @brief This function handles CAN RX1 interrupt.
*/
void CAN1_RX1_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan);
}

/**
* @brief This function handles CAN SCE interrupt.
*/
void CAN1_SCE_IRQHandler(void)
{
  HAL_CAN_IRQHandler(&hcan);
}




