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
  
#include "../hal_modem.h"
#include "hal_ublox_sara_u270.h"
#include "ssUart.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

int32_t muart = -1;

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/


/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

void hal_ublox_sara_u270_init(void)
{
  /* add uart to function arguments */
}

void hal_ublox_sara_u270_power_up(void)
{
  // power up the modem
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, RESET);
  osDelay(50);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, SET);
  osDelay(10);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, RESET);
  osDelay(150);
  HAL_GPIO_WritePin(MDM_PWR_ON_GPIO_Port, MDM_PWR_ON_Pin, SET);
  osDelay(100);
}

void hal_ublox_sara_u270_reset(void)
{
  HAL_GPIO_WritePin(MDM_RESET_GPIO_Port, MDM_RESET_Pin, GPIO_PIN_RESET);
  osDelay(1000);
  HAL_GPIO_WritePin(MDM_RESET_GPIO_Port, MDM_RESET_Pin, GPIO_PIN_SET);
}


uint32_t hal_ublox_sara_u270_read(int32_t fd, uint8_t *buf, uint32_t size, uint32_t timeout)
{
  return ssUartRead(fd, buf, size, timeout);
}

uint32_t hal_ublox_sara_u270_write(int32_t fd, const uint8_t *buf, uint32_t size)
{
  return ssUartWrite(fd, buf, size);
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/


#ifdef __cplusplus
}
#endif


 
