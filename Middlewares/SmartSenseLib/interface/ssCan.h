/**
 * @file     ssCan.h
 * @brief    CAN bus
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_CAN_H
#define _SS_CAN_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
#define SS_CAN_OK                       0
#define SS_CAN_ERR                      1  
  
#define SS_CAN_RX_NEW                   0
#define SS_CAN_RX_EMPTY                 1 
  
#define SS_CAN_BUFFER_SIZE              16
  
#define SS_CAN_BAUDRATE_500000          1
#define SS_CAN_BAUDRATE_100000          5
#define SS_CAN_BAUDRATE_50000           10
#define SS_CAN_BAUDRATE_10000           50
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

uint32_t ssCanInit(uint32_t can_count);
uint32_t ssCanOpen(CAN_TypeDef* CAN, uint32_t buffer_size, uint8_t baud_rate);
uint32_t ssCanTransmit(uint32_t StdId, uint32_t DLC, uint8_t *Data);
uint32_t ssCanReceiveQueue( CanRxMsgTypeDef *canRxBuff );
uint32_t ssCanTransmitQueue( void );
uint32_t ssCanFloat2Bytes(uint8_t* bytes_temp, float float_variable);
uint32_t ssCanFilterSetup( CAN_FilterConfTypeDef  CAN_FilterInitStructure );

#ifdef __cplusplus
}
#endif

#endif /* _SS_UART_H */

