/**
 * @file     AaConfig_rad.c
 * @brief    AaConfig R&D parameter implementation. 
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

/*------------------------- INCLUDED FILES ************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "ssUart.h"
#include "fifo.h"


#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

#define USART_FIFO_USED
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

typedef struct
{
  USART_TypeDef* usart;
  FifoHandle_t rxfifo;
  FifoHandle_t txfifo;
  uint32_t rx_dropped;
} ssUartType;
  
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

ssUartType *m_uarts = NULL;
uint32_t m_uart_count = 0;

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

void ssUartInterrupt(USART_TypeDef* USARTx);


/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
uint32_t ssUartInit(uint32_t uart_count)
{
  uint32_t i;
  m_uarts = pvPortMalloc(uart_count * sizeof(ssUartType));
  if(m_uarts == NULL)
  {
    return DDAL_UART_ERR;
  }
  
  m_uart_count = uart_count;
  
  for(i=0U; i<uart_count; i++)
  {
    m_uarts[i].usart = NULL;
    //m_uarts[i].rx_queue = NULL;
    //m_uarts[i].tx_queue = NULL;
  }
  
  return DDAL_UART_OK;
}


int32_t ssUartOpen(USART_TypeDef* USARTx,
                        ssUartConfigType *config,
                        uint32_t buffer_size)
{
  UART_HandleTypeDef UARTHandle = {0};
  uint32_t i;
  int32_t id = -1;
  
  /* find free slot */
  for(i=0U; i<m_uart_count; i++)
  {
    if(m_uarts[i].usart == NULL)
    {
      id = i;
      break;
    }
    else if (m_uarts[i].usart == USARTx)
    {
      /* uart already in use */
      goto ssUartOpen_err_0;
    }
  }
  
  /* no free slots */
  if(id == -1)
  {
    goto ssUartOpen_err_0;
  }

  m_uarts[id].usart = USARTx;

  m_uarts[id].rxfifo = fifo_create(NULL, buffer_size);
  if(m_uarts[id].rxfifo == NULL)
  {
    goto ssUartOpen_err_1;
  }
  m_uarts[id].txfifo = fifo_create(NULL, buffer_size);
  if(m_uarts[id].txfifo == NULL)
  {
    goto ssUartOpen_err_2;
  }
  m_uarts[id].rx_dropped = 0;
 
  /* Fill default settings */
  UARTHandle.Instance = USARTx;
  UARTHandle.Init.BaudRate = config->baudrate;
  UARTHandle.Init.HwFlowCtl = config->FlowControl;
  UARTHandle.Init.Mode = config->Mode;
  UARTHandle.Init.Parity = config->Parity;
  UARTHandle.Init.StopBits = config->StopBits;
  UARTHandle.Init.WordLength = config->WordLength;
  UARTHandle.Init.OverSampling = UART_OVERSAMPLING_16;
#if defined(STM32F303xE) || defined(STM32L4PLUS)
  UARTHandle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  UARTHandle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
#endif 


  if(HAL_UART_Init(&UARTHandle) != HAL_OK)
  {
    goto ssUartOpen_err_3;
  }
  
#if defined(STM32L4PLUS)
  if (HAL_UARTEx_SetTxFifoThreshold(&UARTHandle, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    goto ssUartOpen_err_4;
  }

  if (HAL_UARTEx_SetRxFifoThreshold(&UARTHandle, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    goto ssUartOpen_err_4;
  }

  if (HAL_UARTEx_DisableFifoMode(&UARTHandle) != HAL_OK)
  {
    goto ssUartOpen_err_4;
  }
#endif

  /* Enable RX interrupt */
#if defined(STM32L4PLUS)
  USARTx->CR1 |= USART_CR1_RXNEIE_RXFNEIE;
#else
  USARTx->CR1 |= USART_CR1_RXNEIE;
#endif
  return id;
  
#if defined(STM32L4PLUS)
ssUartOpen_err_4:
  HAL_UART_DeInit(&UARTHandle);
#endif
ssUartOpen_err_3:
  fifo_destroy(m_uarts[id].txfifo);
ssUartOpen_err_2:
  fifo_destroy(m_uarts[id].rxfifo);
ssUartOpen_err_1:
  m_uarts[id].usart = NULL;
ssUartOpen_err_0: 
  return -1;
}


uint8_t ssUartGetc(uint32_t id, uint32_t timeout)
{
  uint8_t c;
  
  if(fifo_read(m_uarts[id].rxfifo, &c, 1, timeout) != 1)
  {
    /* nothing received */
    c = 0;
  }

  return c;
}


uint32_t ssUartGets(uint32_t id, uint8_t *s, uint32_t size, uint32_t timeout)
{
  uint32_t i;
  uint8_t c = 0;
  TickType_t ticksOld;
  TickType_t ticksNow;
  TickType_t ticksDelta;
  TickType_t delta;
   
  if(timeout != portMAX_DELAY)
  {
    ticksOld = xTaskGetTickCount();
    for(i=0; i<size-1; i++)
    {
      ticksNow = xTaskGetTickCount();
      ticksDelta = ticksNow - ticksOld;
      ticksOld = ticksNow;
      delta = OS_TICKS_TO_MILLISECONDS(ticksDelta);
      if(timeout > delta)
      {
        c = ssUartGetc(id, timeout);
        if(c == 0 || c == '\n')
        {
          break;
        }
        else
        {
          *s = c;
          s++;
        }
      }
    }
  }
  else
  {
    for(i=0; i<size-1; i++)
    {
      c = ssUartGetc(id, timeout);
      if(c == 0 || c == '\n')
      {
        break;
      }
      else
      {
        *s = c;
        s++;
      }
    }
  }
  /* terminate string */
  *s = '\0';
  
  return i;
}

uint8_t ssUartPutc(uint32_t id, const uint8_t c)
{
  return ssUartWrite(id, &c, 1);
}

uint8_t ssUartPuts(uint32_t id, const uint8_t *s)
{
  if(id < m_uart_count)
  {
    /* Go through entire string */
    while (*s != '\0') 
    {
      ssUartPutc(id, *s++);
    }
  }
  return 0;
}

uint32_t ssUartWrite(uint32_t id, const uint8_t *s, const uint32_t size)
{
  uint32_t nbytes = 0;

  if(id >= m_uart_count)
  {
    return 0;
  }
  
  while(size > nbytes)
  {
    nbytes += fifo_write(m_uarts[id].txfifo, &s[nbytes], size - nbytes, 0);
    BSP_UART_START_TX(m_uarts[id].usart);
  }

  return nbytes;
}


uint32_t ssUartRead(uint32_t id, uint8_t *s, const uint32_t size, uint32_t timeout)
{
  uint32_t nbytes = 0;
  if(id >= m_uart_count)
  {
    return 0;
  }

  nbytes = fifo_read(m_uarts[id].rxfifo, s, size, timeout);

  return nbytes;
}



/**
* @brief This function handles USART1 global interrupt.
*/
void USART1_IRQHandler(void)
{
  ssUartInterrupt(USART1);
}

void USART2_IRQHandler(void)
{
  ssUartInterrupt(USART2);
}

#if BSP_UART3_ENABLED == BSP_ON
void USART3_IRQHandler(void)
{
  ssUartInterrupt(USART3);
}
#endif

#if defined(BSP_USART3_ENABLED)
void USART3_IRQHandler(void)
{
  ssUartInterrupt(USART3);
}
#endif

#if defined(BSP_USART6_ENABLED)
void USART6_IRQHandler(void)
{
  ssUartInterrupt(USART6);
}
#endif


#if defined(BSP_UART5_ENABLED)
void UART5_IRQHandler(void)
{
  ssUartInterrupt(UART5);
}
#endif

#if defined(BSP_UART7_ENABLED)
void UART7_IRQHandler(void)
{
  ssUartInterrupt(UART7);
}
#endif

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

void ssUartInterrupt(USART_TypeDef* USARTx)
{
  uint32_t i;
  int32_t id = -1;
  uint8_t c;
  
  /* find the uart id */
  for(i=0; i<m_uart_count; i++)
  {
    if(m_uarts[i].usart == USARTx)
    {
      id = i;
      break;
    }
  }
    
  if(id != -1)
  {
    if(BSP_USART_RX_INT(USARTx)) 
    {
      /* received byte */
      c = BSP_USART_READ_DATA(USARTx);
      fifo_write(m_uarts[id].rxfifo, &c, 1, 0);
    }      
    if(BSP_USART_TX_INT(USARTx))
    {
      /* uart data register is empty, we can send new byte if available */
      if(fifo_read(m_uarts[id].txfifo, &c, 1, 0) == 1)
      {
        BSP_USART_WRITE_DATA(USARTx, c);
      }
      else
      {
        /* no more data, stop transmission */
        BSP_UART_STOP_TX(USARTx);
      }
    }
  }
}


#ifdef __cplusplus
}
#endif



