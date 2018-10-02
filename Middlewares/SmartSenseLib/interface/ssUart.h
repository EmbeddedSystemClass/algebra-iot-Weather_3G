/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_UART_H
#define _SS_UART_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "queue.h"

#include "bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

#define DDAL_UART_OK    0
#define DDAL_UART_ERR   1
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

typedef struct
{
  uint32_t baudrate;
  uint32_t FlowControl;
  uint32_t Mode;
  uint32_t Parity;
  uint32_t StopBits;
  uint32_t WordLength;
} ssUartConfigType;

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

uint32_t ssUartInit(uint32_t uart_count);

int32_t ssUartOpen(USART_TypeDef* USARTx,
                   ssUartConfigType *config,
                   uint32_t buffer_size);

uint8_t ssUartGetc(uint32_t id, uint32_t timeout);
uint32_t ssUartGets(uint32_t id, uint8_t *s, uint32_t size, uint32_t timeout);
uint8_t ssUartPutc(uint32_t id, const uint8_t c);
uint8_t ssUartPuts(uint32_t id, const uint8_t *s);
uint32_t ssUartWrite(uint32_t id, const uint8_t *s, const uint32_t size);
uint32_t ssUartRead(uint32_t id, uint8_t *s, const uint32_t size, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* _SS_UART_H */

