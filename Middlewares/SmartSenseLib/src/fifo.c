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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "semphr.h"     /* for uxSemaphoreGetCount() */

#include "fifo.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef struct fifo_t
{
  uint8_t* buf;              //!< buffer
  uint8_t* abuf;             //!< allocated buffer
  uint32_t   size;             //!< size of buffer (s - 1) elements can be stored
  volatile uint32_t head;     //!< write index
  volatile uint32_t tail;     //!< read index
  volatile uint32_t count;
  osSemaphoreId countSem;
  osSemaphoreId freeSem;
} fifo_t;
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

FifoHandle_t fifo_create(uint8_t *buf, uint32_t size)
{
  fifo_t *newfifo = NULL;
  int i;
  
  if(size > 0)
  {
    newfifo = pvPortMalloc(sizeof(fifo_t));
    if(newfifo != NULL)
    {
      newfifo->abuf = NULL;
      newfifo->buf = buf;
      newfifo->tail = 0;
      newfifo->head = 0;
      newfifo->size = size;
      newfifo->count = 0;
      newfifo->countSem = osSemaphoreCreate(NULL, size);
      newfifo->freeSem = osSemaphoreCreate(NULL, size);
      
      /* cmsis doesn't allow semaphore to be create with initial value different than 0,
      * initialize it to max value this way
      */
      for(i=0; i<size; i++)
      {
        osSemaphoreRelease(newfifo->freeSem);
      }
      
      if(buf == NULL)
      {
        newfifo->abuf = pvPortMalloc(size);
        if(newfifo->abuf != NULL)
        {
          newfifo->buf = newfifo->abuf;
          memset(newfifo->buf, 0, size);
        }
        else
        {
          /* failed to allocate buffer, restore fifo memory */
          vPortFree(newfifo);
          newfifo = NULL;
        }
      }
    }
  }
  
  return (FifoHandle_t)newfifo;
}



void fifo_destroy(FifoHandle_t hfifo)
{
  fifo_t *fifo = (fifo_t *)hfifo;
  
  if(fifo != NULL)
  {
    if(fifo->abuf != NULL)
    {
      vPortFree(fifo->abuf);
    }
    
    vPortFree(fifo);
  }
}


uint32_t fifo_length(FifoHandle_t hfifo)
{
  fifo_t *fifo = (fifo_t *)hfifo;
  
  return (uint32_t)uxSemaphoreGetCount(fifo->countSem);
}

uint32_t fifo_free(FifoHandle_t hfifo)
{
  fifo_t *fifo = (fifo_t *)hfifo;
  
  return (uint32_t)uxSemaphoreGetCount(fifo->freeSem);
}


uint32_t fifo_write(FifoHandle_t hfifo, const uint8_t *buf, uint32_t nbytes, uint32_t timeout)
{
  fifo_t *fifo = (fifo_t *)hfifo;
  uint32_t n;
  
  for(n=0; n<nbytes; n++)
  {
    if(osSemaphoreWait(fifo->freeSem, timeout) == osOK)
    {
      fifo->count++;
      fifo->buf[fifo->head] = buf[n];
      fifo->head++;
      if(fifo->head == fifo->size)
      {
        fifo->head = 0;
      }
      osSemaphoreRelease(fifo->countSem);
    }
    else
    {
      break;
    }
  }
  
  return n;
}


uint32_t fifo_read(FifoHandle_t hfifo, uint8_t *buf, uint32_t nbytes, uint32_t timeout)
{
  fifo_t *fifo = (fifo_t *)hfifo;
  uint32_t n;
  uint32_t count = 0;
  int32_t ret;
  bool error = false;
  
  for(n=0; (n<nbytes) && !error; n++)
  {
    ret = osSemaphoreWait(fifo->countSem, timeout);
    if(ret == osOK)
    {
      fifo->count--;
      buf[n] = fifo->buf[fifo->tail];
      fifo->tail++;
      if(fifo->tail == fifo->size)
      {
        fifo->tail = 0;
      }
      osSemaphoreRelease(fifo->freeSem);
      count++;
    }
    else
    {
      error = true;
    }
  }
  
  return count;
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

#ifdef __cplusplus
}
#endif



