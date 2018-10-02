/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _FIFO_H
#define _FIFO_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

typedef const void * FifoHandle_t;

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

FifoHandle_t fifo_create(uint8_t *buf, uint32_t size);
void fifo_destroy(FifoHandle_t hfifo);

uint32_t fifo_write(FifoHandle_t hfifo, const uint8_t *buf, uint32_t nbytes, uint32_t timeout);

uint32_t fifo_read(FifoHandle_t hfifo, uint8_t *buf, uint32_t nbytes, uint32_t timeout);

uint32_t fifo_length(FifoHandle_t hfifo);


/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/


#ifdef __cplusplus
}
#endif

#endif /* _FIFO_H */

