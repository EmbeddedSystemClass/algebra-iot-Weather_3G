/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2018. All rights reserved.
 *
 **/

#ifndef _SS_WIFI_H
#define _SS_WIFI_H

#include <stdint.h>

#include "FreeRTOS.h"

#include "bsp.h"

#include "wlan.h"
#include "sys/socket.h"
#include "netapp.h"
#include "ssDevMan.h"

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef struct wifi_dev_t
{
    netif_t *com_dev;
} wifi_dev_t;
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

void initWifiModem();

int32_t establishConnectionWithAP();
int32_t checkInternetConnection();
void SimpleLinkNetAppEventHandler(SlNetAppEvent_t *pNetAppEvent);
void SimpleLinkWlanEventHandler(SlWlanEvent_t *pWlanEvent);
void SimpleLinkPingReport(SlPingReport_t *pPingReport);
int32_t checkLanConnection();
void SimpleLinkGeneralEventHandler(SlDeviceEvent_t *pDevEvent);

#ifdef __cplusplus
}
#endif

#endif /* _SS_WIFI_H */

