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

#include <stdint.h>
#include <stdbool.h>
  
#include "cmsis_os.h"
  
#include "hal_modem.h"

  
#if !defined(MODEM_USED) 
  #warning "MODEM_USED variable not set, assuming modem is not used. Exclude file from compilation if modem is not used."
#elif (MODEM_USED == NO)   
  #warning "MODEM_USED variable set to NO. Exclude file from compilation if modem is not used."
#elif !defined(MODEM_TYPE)
  #error "MODEM_TYPE variable not set."
#elif (MODEM_TYPE == MODEM_TYPE_UBLOX_SARA_U270)
  #include "ublox/hal_ublox_sara_u270.h"
#elif (MODEM_TYPE == MODEM_TYPE_UBLOX_SARA_N211)
  #include "ublox/hal_ublox_sara_n211.h"
#else
  #error "MODEM_TYPE variable set to unknown value."
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

void (*hal_modem_power_up_ptr)() = NULL;
void (*hal_modem_reset_ptr)() = NULL;
uint32_t (*hal_modem_read_ptr)(int32_t, uint8_t *, uint32_t, uint32_t) = NULL;
uint32_t (*hal_modem_write_ptr)(int32_t, const uint8_t *, uint32_t) = NULL;

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/


/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

void hal_modem_init(void)
{
#if (MODEM_USED == YES)
#if (defined(MODEM_TYPE))
#if (MODEM_TYPE == MODEM_TYPE_UBLOX_SARA_U270)
  hal_ublox_sara_u270_init();
  hal_modem_power_up_ptr = hal_ublox_sara_u270_power_up;
  hal_modem_reset_ptr = hal_ublox_sara_u270_reset;
  hal_modem_read_ptr = hal_ublox_sara_u270_read;
  hal_modem_write_ptr = hal_ublox_sara_u270_write;
#elif (MODEM_TYPE == MODEM_TYPE_UBLOX_SARA_N211)
  hal_modem_power_ptr = hal_ublox_sara_n211_power;
  hal_modem_reset_ptr = hal_ublox_sara_n211_reset;
  hal_modem_read_ptr = hal_ublox_sara_n211_read;
  hal_modem_write_ptr = hal_ublox_sara_n211_write;
#endif
#endif
#endif
}

void hal_modem_power_up()
{
  if(hal_modem_power_up_ptr)
  {
    hal_modem_power_up_ptr();
  }
}

void hal_modem_reset(void)
{
  if(hal_modem_reset_ptr)
  {
    hal_modem_reset_ptr();
  }
}

uint32_t hal_modem_read(int32_t fd, uint8_t *buf, uint32_t size, uint32_t timeout)
{
  if(hal_modem_read_ptr)
  {
    return hal_modem_read_ptr(fd, buf, size, timeout);
  }
  else
  {
    return 0;
  }
}

uint32_t hal_modem_write(int32_t fd, const uint8_t *buf, uint32_t size)
{
  if(hal_modem_write_ptr)
  {
    return hal_modem_write_ptr(fd, buf, size);
  }
  else
  {
    return 0;
  }
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/


#ifdef __cplusplus
}
#endif


 
