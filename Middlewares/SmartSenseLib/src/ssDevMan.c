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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
  
#include "sys/socket.h"
#include "netinet/in.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "ATCmdParser.h"

#include "ssWifi.h"
#include "ssWifiWrapper.h"
#include "ssModemWrapper.h"

#include "ssDevMan.h"
#include "ssModem.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

netif_t *get_device(void)
{

#ifndef WIFI
  return (netif_t *)s_init_modem();
#else
  return (netif_t *)s_init_wifi_dev();
#endif

}


/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
