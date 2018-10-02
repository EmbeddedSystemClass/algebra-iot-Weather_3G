/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_SI70XX_H
#define _SS_SI70XX_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
#define SS_SI70XX_OK  0
#define SS_SI70XX_ERR 1
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

void ssSi70xx_Init(void); 
int32_t ssSi70xx_Open(I2C_HandleTypeDef *i2c);  
void ssSi70xx_Close(int32_t id);
uint32_t ssSi70xx_ReadTemperature(int32_t id, uint32_t timeout, uint16_t *value);
uint32_t ssSi70xx_ReadHumidity(int32_t id, uint32_t timeout, uint16_t *value); 

#ifdef __cplusplus
}
#endif

#endif /* _SS_SI70XX_H */
 