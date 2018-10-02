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
  
#include "bsp.h"
#include "ssSi70xx.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

#define SS_SI70XX_I2C_ADDRESS   0x80  
#define SS_SI70XX_READY_RETRY   10  
#define SS_SI70XX_TIMEOUT       100
  
#define I2C_BUFFER_SIZE 2
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef struct ssSi70xxDataType
{
  I2C_HandleTypeDef *i2c;
} ssSi70xxDataType;  
  
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

ssSi70xxDataType ssSi70xxData[BSP_SI70XX_SENS_CNT];

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
  
void ssSi70xx_Init(void)
{
  int32_t i;
  
  for(i=0; i<BSP_SI70XX_SENS_CNT; i++)
  {
    ssSi70xxData[i].i2c = NULL;
  }
}

int32_t ssSi70xx_Open(I2C_HandleTypeDef *i2c)
{
  int32_t i;
  int32_t id = -1;
  uint8_t sens_status;
  uint8_t buff[I2C_BUFFER_SIZE] = {0};
  
  for(i=0; i<BSP_SI70XX_SENS_CNT; i++)
  {
    if(ssSi70xxData[i].i2c == NULL)
    {
      id = i;
      ssSi70xxData[i].i2c = i2c;
      break;
    }
    else if(ssSi70xxData[i].i2c == i2c)
    {
      break;
    }
  }
  
  if(id == -1)
  {
    goto ssSi70xx_Open_err_0;
  }
    
  if(HAL_I2C_IsDeviceReady(i2c, SS_SI70XX_I2C_ADDRESS, SS_SI70XX_READY_RETRY , SS_SI70XX_TIMEOUT) != HAL_OK)
  {
    goto ssSi70xx_Open_err_1;
  }    
  
  if(HAL_I2C_Mem_Read(i2c, SS_SI70XX_I2C_ADDRESS, 0xE7, I2C_MEMADD_SIZE_8BIT, buff, 1, SS_SI70XX_TIMEOUT) != HAL_OK)
  {
    goto ssSi70xx_Open_err_1;
  }
  
  sens_status = buff[0];
  sens_status &= ~0xC0;
  sens_status |= (0 << 6) & 0xC0;
  sens_status &= ~0x04;
  sens_status |= (0 << 2) & 0x04;
  if(HAL_I2C_Mem_Write(i2c, 0x80, 0xE6, I2C_MEMADD_SIZE_8BIT, &sens_status, 1, 100) != HAL_OK)
  {
    goto ssSi70xx_Open_err_1;
  }
  
  return id;
ssSi70xx_Open_err_1:
  ssSi70xxData[id].i2c = NULL;
ssSi70xx_Open_err_0:
  return -1;
}

void ssSi70xx_Close(int32_t id)
{
  if(id < BSP_SI70XX_SENS_CNT)
  {
    ssSi70xxData[id].i2c = NULL;
  }
}

uint32_t ssSi70xx_ReadTemperature(int32_t id, uint32_t timeout, uint16_t *value)
{
  uint32_t status = SS_SI70XX_ERR;
  uint8_t buff[I2C_BUFFER_SIZE] = {0};
  
  *value = 0;
  if((id < BSP_SI70XX_SENS_CNT) && (ssSi70xxData[id].i2c != NULL))
  {  
    if(HAL_I2C_Mem_Read(ssSi70xxData[id].i2c, SS_SI70XX_I2C_ADDRESS, 0xE3, I2C_MEMADD_SIZE_8BIT, buff, 2, timeout) == HAL_OK)
    {
      uint16_t temp = (buff[0] <<8) | buff[1];
      *value = (uint16_t)(0.26812744140625 * temp) - 4685;
      status = SS_SI70XX_OK;
    }
  }
  
  return status;
}

uint32_t ssSi70xx_ReadHumidity(int32_t id, uint32_t timeout, uint16_t *value)
{
  uint32_t status = SS_SI70XX_ERR;
  uint8_t buff[I2C_BUFFER_SIZE] = {0};
    
  *value = 0;
  if((id < BSP_SI70XX_SENS_CNT) && (ssSi70xxData[id].i2c != NULL))
  {
    if(HAL_I2C_Mem_Read(ssSi70xxData[id].i2c, SS_SI70XX_I2C_ADDRESS, 0xE5, I2C_MEMADD_SIZE_8BIT, buff, 2, timeout) == HAL_OK)
    {
      uint16_t temp = (buff[0] <<8) | buff[1];
      *value = (uint16_t)(0.19073486328125 * temp) - 600;
      status = SS_SI70XX_OK;
    }
  }  
  return status;
}
/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

#ifdef __cplusplus
}
#endif