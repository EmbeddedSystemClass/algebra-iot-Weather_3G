/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_CO2_SENSOR_H
#define _SS_CO2_SENSOR_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "ssUart.h"

#if defined(USE_CO2_COZIR)
#elif defined(USE_CO2_SH300)
#else
#error "CO2 sensor type must be defined"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
#define SS_CO2_STATUS_OK      0
#define SS_CO2_STATUS_ERR     1 
#define SS_CO2_STATUS_TIMEOUT 2 
  
#define SS_CO2_FLAG_FORCE_CONFIGURE 0x80000000U
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
 
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
uint32_t ssCO2SensorOpen(USART_TypeDef* USARTx, uint32_t flags);

uint16_t ssCO2SensorReadInstantCO2(void);
uint16_t ssCO2SensorReadFilteredCO2(void);

uint32_t ssCO2SensorCalibrate(void);
  
  
/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/


#ifdef __cplusplus
}
#endif

#endif /* _SS_CO2_SENSOR_H */

