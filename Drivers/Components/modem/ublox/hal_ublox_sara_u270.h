/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _HAL_UBLOX_SARA_U270_H
#define _HAL_UBLOX_SARA_U270_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

void hal_ublox_sara_u270_init(void);

void hal_ublox_sara_u270_power_up(void);
void hal_ublox_sara_u270_reset(void);
uint32_t hal_ublox_sara_u270_read(int32_t fd, uint8_t *buf, uint32_t size, uint32_t timeout);
uint32_t hal_ublox_sara_u270_write(int32_t fd, const uint8_t *buf, uint32_t size);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _HAL_UBLOX_SARA_U270_H */
 
