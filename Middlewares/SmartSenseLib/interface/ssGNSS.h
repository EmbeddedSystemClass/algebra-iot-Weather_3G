/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2018. All rights reserved.
 *
 **/
#ifndef _SS_GNSS_H
#define _SS_GNSS_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include <ssUart.h>


#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
#define SS_GNSS_STATUS_OK      0
#define SS_GNSS_STATUS_ERR     1 
#define SS_GNSS_STATUS_TIMEOUT 2 
    
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
struct ssGNSScoords
{
    float lat;
    float lon;
};

struct ssCoord_time
{
    struct ssGNSScoords coords;
    uint64_t timestamp;
};
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
uint8_t ssGNSSinit(USART_TypeDef* USARTx, uint32_t flags);
void ssGNSSRead(void);
bool ssGNSSGetCoords(struct ssGNSScoords *coords);
bool ssGNSSGetTime(char *datetime, size_t size);
uint64_t datetime_to_timestamp(const char *datetime);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _SS_GNSS_H */
