/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _HAL_MODEM_H
#define _HAL_MODEM_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
  
/* Supported modems */
#define MODEM_TYPE_UBLOX_SARA_U270   1
#define MODEM_TAPE_UBLOX_SARA_N211   2
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

void hal_modem_init(void);

void hal_modem_power_up(void);
void hal_modem_reset(void);
uint32_t hal_modem_read(int32_t fd, uint8_t *buf, uint32_t size, uint32_t timeout);
uint32_t hal_modem_write(int32_t fd, const uint8_t *buf, uint32_t size);


/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _HAL_MODEM_H */
 
