/**
  ******************************************************************************
  * @file    EEPROM/EEPROM_Emulation/src/eeprom.c 
  * @author  MCD Application Team
  * @brief   This file provides all the EEPROM emulation firmware functions.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright © 2017 STMicroelectronics International N.V. 
  * All rights reserved.</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without 
  * modification, are permitted, provided that the following conditions are met:
  *
  * 1. Redistribution of source code must retain the above copyright notice, 
  *    this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright notice,
  *    this list of conditions and the following disclaimer in the documentation
  *    and/or other materials provided with the distribution.
  * 3. Neither the name of STMicroelectronics nor the names of other 
  *    contributors to this software may be used to endorse or promote products 
  *    derived from this software without specific written permission.
  * 4. This software, including modifications and/or derivative works of this 
  *    software, must execute solely and exclusively on microcontroller or
  *    microprocessor devices manufactured by or for STMicroelectronics.
  * 5. Redistribution and use of this software other than as permitted under 
  *    this license is void and will automatically terminate your rights under 
  *    this license. 
  *
  * THIS SOFTWARE IS PROVIDED BY STMICROELECTRONICS AND CONTRIBUTORS "AS IS" 
  * AND ANY EXPRESS, IMPLIED OR STATUTORY WARRANTIES, INCLUDING, BUT NOT 
  * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A 
  * PARTICULAR PURPOSE AND NON-INFRINGEMENT OF THIRD PARTY INTELLECTUAL PROPERTY
  * RIGHTS ARE DISCLAIMED TO THE FULLEST EXTENT PERMITTED BY LAW. IN NO EVENT 
  * SHALL STMICROELECTRONICS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
  * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
  * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
  * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/** @addtogroup EEPROM_Emulation
  * @{
  */ 

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include <string.h>
#include "assert.h"

#include "cmsis_os.h"

#include "ssLogging.h"
    
#include "eeprom.h"

/* Private typedef -----------------------------------------------------------*/
    
typedef struct nv_record_s
{
  uint16_t id;          /* record ID (used by upper layer for addressing) */
  uint16_t size;        /* data size in bytes */
  uint8_t data[0];      /* name says it all */
} nv_record_s;
    
typedef struct nv_store_entry_s
{
  uint16_t id;
  uint32_t addr;
} nv_store_entry_s;

typedef struct nv_store_s
{
  uint32_t ptr;         /* Last updated record offset from the page start */
  uint32_t page;        /* Current active page */
  uint32_t base;        /* Current active page start address */
  uint32_t end;         /* Current active page end address */
  osMutexId lock;
  uint32_t map_size;  
  nv_store_entry_s *map;      /* id to address map */ 
} nv_store_s;


/* Private define ------------------------------------------------------------*/

#define INVALID_ADDRESS 0xFFFFFFFF
#define INVALID_ID      0xFFFF

/* NV storage state */
#define NV_STATE_OK                     0
#define NV_STATE_NOTINITIALIZED         1
#define NV_STATE_TRANSFER_IN_PROGRESS   2
#define NV_STATE_TRANSFER_DONE          3
#define NV_STATE_CORRUPTED              4

#define NV_DATA_ALIGNMENT  2

#define NV_GET_DATA_SIZE(x) (((x) + (NV_DATA_ALIGNMENT-1)) & (~(NV_DATA_ALIGNMENT-1)))

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* Global variable used to store variable value in read sequence */
uint16_t DataVar = 0;


static nv_store_s nv_store;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
static HAL_StatusTypeDef EE_Format(void);

static bool nv_switch_pages(nv_store_s *nv, uint16_t skip_id);
static bool nv_is_page_erased(uint32_t Address);

/**
  * @brief  Restore the pages to a known good state in case of page's status
  *   corruption after a power loss.
  * @param  None.
  * @retval - Flash error code: on write Flash error
  *         - FLASH_COMPLETE: on success
  */
uint32_t nv_init(uint32_t size)
{
  uint32_t i;
  uint16_t PageStatus0 = 6, PageStatus1 = 6;
  HAL_StatusTypeDef  FlashStatus;
  uint32_t SectorError = 0;
  FLASH_EraseInitTypeDef pEraseInit;
  osMutexDef_t mutex_def;
  uint32_t nv_state;
  uint32_t primary_page;
  uint32_t secondary_page;
  
  /* Get Page0 status */
  PageStatus0 = (*(__IO uint16_t*)PAGE0_BASE_ADDRESS);
  /* Get Page1 status */
  PageStatus1 = (*(__IO uint16_t*)PAGE1_BASE_ADDRESS);
 
  HAL_FLASH_Unlock();
  
  
  if(PageStatus0 == ERASED)
  {
    if(PageStatus1 == ERASED)
    {
      /* first use; not initialized */
      nv_state = NV_STATE_NOTINITIALIZED;
      primary_page = PAGE0_ID;
      secondary_page = PAGE1_ID;
    }
    else if(PageStatus1 == RECEIVE_DATA)
    {
      /* Data was transfered to secondary page but the erasing of former primary page was interrupted */
      nv_state = NV_STATE_TRANSFER_DONE;
      primary_page = PAGE1_ID;
      secondary_page = PAGE0_ID;
    }
    else if(PageStatus1 == VALID_PAGE)
    {
      nv_state = NV_STATE_OK;
      primary_page = PAGE1_ID;
      secondary_page = PAGE0_ID;
    } 
    else
    {
      /* Page 1 corrupted */
      nv_state = NV_STATE_CORRUPTED;
      primary_page = PAGE0_ID;
      secondary_page = PAGE1_ID;
    }
  }
  else if(PageStatus0 == RECEIVE_DATA)
  {
    if(PageStatus1 == ERASED)
    {
      /* Data was transfered to secondary page but the erasing of former primary page was interrupted */
      nv_state = NV_STATE_TRANSFER_DONE;
      primary_page = PAGE0_ID;
      secondary_page = PAGE1_ID;
    }
    else if(PageStatus1 == RECEIVE_DATA)
    {
      /* Not valid */
      nv_state = NV_STATE_CORRUPTED;
      primary_page = PAGE0_ID;
      secondary_page = PAGE1_ID;
    }
    else if(PageStatus1 == VALID_PAGE)
    {
      nv_state = NV_STATE_TRANSFER_IN_PROGRESS;
      primary_page = PAGE1_ID;
      secondary_page = PAGE0_ID;
    } 
    else
    {
      /* Page 1 corrupted */
      nv_state = NV_STATE_CORRUPTED;
      primary_page = PAGE0_ID;
      secondary_page = PAGE1_ID;
    }
  }
  else if(PageStatus0 == VALID_PAGE)
  {
    if(PageStatus1 == ERASED)
    {
      nv_state = NV_STATE_OK;
      primary_page = PAGE0_ID;
      secondary_page = PAGE1_ID;
    }
    else if(PageStatus1 == RECEIVE_DATA)
    {
      /* transfer interrupted */
      nv_state = NV_STATE_TRANSFER_IN_PROGRESS;
      primary_page = PAGE0_ID;
      secondary_page = PAGE1_ID;
    }
    else if(PageStatus1 == VALID_PAGE)
    {
      /* Invalid */
      nv_state = NV_STATE_CORRUPTED;
      primary_page = PAGE0_ID;
      secondary_page = PAGE1_ID;
    } 
    else
    {
      /* Page 1 corrupted */
      nv_state = NV_STATE_CORRUPTED;
      primary_page = PAGE0_ID;
      secondary_page = PAGE1_ID;
    }
  }
  else
  {
    /* page 0 corrupted */
    nv_state = NV_STATE_CORRUPTED;
    primary_page = PAGE0_ID;
    secondary_page = PAGE1_ID;
  }
 
  /* init nv_store data */
  nv_store.map_size = size;
  nv_store.map = pvPortMalloc(sizeof(nv_store_entry_s)*size);
  assert(nv_store.map);
 
  for(i=0; i<nv_store.map_size; i++)
  {
    nv_store.map[i].id = INVALID_ID;
    nv_store.map[i].addr = INVALID_ADDRESS;
  }
  nv_store.lock = osMutexCreate(&mutex_def);
  assert(nv_store.lock);
  
  if(primary_page == PAGE0_ID)
  {
    nv_store.base = PAGE0_BASE_ADDRESS;
    nv_store.end = PAGE0_END_ADDRESS;
    nv_store.page = PAGE0_ID;
  }
  else
  {
    nv_store.base = PAGE1_BASE_ADDRESS;
    nv_store.end = PAGE1_END_ADDRESS;
    nv_store.page = PAGE1_ID;
  }
  
  nv_store.ptr = (nv_store.base + 2);
  
  /* get variables' current addresses if storage is valid */
  if((nv_state == NV_STATE_OK) || 
     (nv_state == NV_STATE_TRANSFER_IN_PROGRESS) ||
     (nv_state == NV_STATE_TRANSFER_DONE))
  {
    /* traverse through the page and remember the last address for each variable */
    __IO nv_record_s *rec = (__IO nv_record_s*)(nv_store.ptr);
    
    while((rec < (__IO nv_record_s*)nv_store.end) && (rec->id != INVALID_ID))
    {
      if(rec->id < nv_store.map_size)
      {
        nv_store.map[rec->id].id = rec->id;
        nv_store.map[rec->id].addr = (uint32_t)rec;
        
        /* move to the next record */
        rec = (__IO nv_record_s*)((uint8_t *)rec + sizeof(nv_record_s) + NV_GET_DATA_SIZE(rec->size));
      }
      else
      {
        /* error: id out of range */
        assert(0);
      }
    }
    nv_store.ptr = (uint32_t)rec;
  }
    
  if((nv_state == NV_STATE_OK) || 
     (nv_state == NV_STATE_TRANSFER_DONE))
  {
    /* NV storage is in valid state */
  }
  else if(nv_state == NV_STATE_TRANSFER_IN_PROGRESS)
  {
    pEraseInit.TypeErase = TYPEERASE_SECTORS;
    pEraseInit.Sector = PAGE0_ID;
    pEraseInit.NbSectors = 1;
    pEraseInit.VoltageRange = VOLTAGE_RANGE;
  }
  else if((nv_state == NV_STATE_CORRUPTED) || 
          (nv_state == NV_STATE_NOTINITIALIZED))
  {
    /* First EEPROM access (Page0&1 are erased) or invalid state -> format EEPROM */
    FlashStatus = EE_Format();
  }
  else
  {
    /* should not end up here */
    assert(0);
  }

  return HAL_OK;
}


int nv_read(uint16_t id, void* ptr, uint16_t size)
{
  int ret = -1;
  nv_record_s *rec;
  uint32_t nbytes;
  
  ssLoggingPrint(ESsLoggingLevel_Debug, LOGGING_NO_CATEGORY, "NV read id=%d, size=%d", id, size);
  
  if(osMutexWait(nv_store.lock, osWaitForever) == osOK)
  {
    if(nv_store.map[id].addr != INVALID_ADDRESS)
    {
      rec = (nv_record_s *)nv_store.map[id].addr;
      nbytes = size < rec->size ? size : rec->size;
      memcpy(ptr, rec->data, nbytes);
      ret = nbytes;
      
      ssLoggingPrintRawStr(ESsLoggingLevel_Debug, LOGGING_NO_CATEGORY,
                           ptr,
                           nbytes,
                           "NV read from 0x%x id=%d, size=%d", 
                           nv_store.map[id].addr,
                           id,
                           nbytes);
    }
    osMutexRelease(nv_store.lock);
  }
  
  return ret;
}

int nv_write(uint16_t id, void* ptr, uint16_t size)
{
  int i;
  int ret = -1;
  bool status = true;
  HAL_StatusTypeDef FlashStatus;
  
  ssLoggingPrintRawStr(ESsLoggingLevel_Debug, LOGGING_NO_CATEGORY,
                         ptr,
                         size,
                         "NV write id=%d, size=%d", 
                         id,
                         size);
  
  if(osMutexWait(nv_store.lock, osWaitForever) == osOK)
  {
    
    /* check if there is enough space on current page */
    if(nv_store.ptr + sizeof(nv_record_s) + size >= nv_store.end)
    {
      status = nv_switch_pages(&nv_store, id);
    }
    
    /* now write the variable */
    
    if(status)
    {
      uint32_t address = nv_store.ptr;
      
      /* write id and address */
      if((HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, address, id) == HAL_OK) &&
         (HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, address + 2, size) == HAL_OK))
      {
        address += 4;
        for(i=0; i<size; i++)
        {
          uint8_t byte;
          byte = ((uint8_t*)ptr)[i];
          if((FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_BYTE, address, byte)) != HAL_OK) break;
          address++;
        }
        if(FlashStatus == HAL_OK)
        {
          ret = i;
          nv_store.map[id].addr = nv_store.ptr;
          ssLoggingPrint(ESsLoggingLevel_Debug, LOGGING_NO_CATEGORY, "NV written to 0x%x id=%d, size=%d", nv_store.map[id].addr, id, i);
          nv_store.ptr += sizeof(nv_record_s) + NV_GET_DATA_SIZE(size);
        }
      }
      
    }
    
    
    
    osMutexRelease(nv_store.lock);
  }
  
  return ret;
}


/**
  * @brief  Erases PAGE and PAGE1 and writes VALID_PAGE header to PAGE
  * @param  None
  * @retval Status of the last operation (Flash write or erase) done during
  *         EEPROM formating
  */
static HAL_StatusTypeDef EE_Format(void)
{
  HAL_StatusTypeDef FlashStatus = HAL_OK;
  uint32_t SectorError = 0;
  FLASH_EraseInitTypeDef pEraseInit;

  pEraseInit.TypeErase = FLASH_TYPEERASE_SECTORS;  
  pEraseInit.Sector = PAGE0_ID;
  pEraseInit.NbSectors = 1;
  pEraseInit.VoltageRange = VOLTAGE_RANGE;
  /* Erase Page0 */
  if(!nv_is_page_erased(PAGE0_BASE_ADDRESS))
  {
    FlashStatus = HAL_FLASHEx_Erase(&pEraseInit, &SectorError); 
    /* If erase operation was failed, a Flash error code is returned */
    if (FlashStatus != HAL_OK)
    {
      return FlashStatus;
    }
  }
  /* Set Page0 as valid page: Write VALID_PAGE at Page0 base address */
  FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, PAGE0_BASE_ADDRESS, VALID_PAGE); 
  /* If program operation was failed, a Flash error code is returned */
  if (FlashStatus != HAL_OK)
  {
    return FlashStatus;
  }

  pEraseInit.Sector = PAGE1_ID;
  /* Erase Page1 */
  if(!nv_is_page_erased(PAGE1_BASE_ADDRESS))
  {  
    FlashStatus = HAL_FLASHEx_Erase(&pEraseInit, &SectorError); 
    /* If erase operation was failed, a Flash error code is returned */
    if (FlashStatus != HAL_OK)
    {
      return FlashStatus;
    }
  }
  
  return HAL_OK;
}




static bool nv_switch_pages(nv_store_s *nv, uint16_t id)
{
  uint32_t old_page;
  uint32_t new_page;
  uint32_t NewPageAddress;
  HAL_StatusTypeDef FlashStatus;
  int i;
  uint32_t SectorError = 0;
  FLASH_EraseInitTypeDef pEraseInit;
  
  ssLoggingPrint(ESsLoggingLevel_Debug, LOGGING_NO_CATEGORY, "NV switch pages");
  
  if(nv->page == PAGE0_ID)
  {
    old_page = PAGE0_ID;
    new_page = PAGE1_ID;
    NewPageAddress = PAGE1_BASE_ADDRESS;
  }
  else
  {
    old_page = PAGE1_ID;
    new_page = PAGE0_ID;
    NewPageAddress = PAGE0_BASE_ADDRESS;
  }
  
  /* Set the new Page status to RECEIVE_DATA status */
  if((FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, NewPageAddress, RECEIVE_DATA)) != HAL_OK) return false;
  
  nv_store.ptr = NewPageAddress + 2;
  for(i=0; i<nv_store.map_size; i++)
  {
    __IO nv_record_s *rec;
    if((nv_store.map[i].addr != INVALID_ADDRESS) && (i != id))
    {
      uint16_t size;
      int k;
      uint32_t adr;
      
      rec =  (__IO nv_record_s *)nv_store.ptr;
      size = rec->size;
      adr = nv_store.ptr;
      
      /* write id and address */
      if((FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, adr, nv_store.map[i].id)) != HAL_OK) break;
      adr += 2;
      
      if((FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, adr, size)) != HAL_OK) break;
      adr += 2;
      
      for(k=0; k<size; i++)
      {
        uint8_t byte;
        
        byte = rec->data[k];
        if((FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_BYTE, adr, byte)) != HAL_OK) break;
        adr++;
      }
        
      if(FlashStatus != HAL_OK) break;
        
      nv_store.map[i].addr = nv_store.ptr;
      nv_store.ptr = adr;
    }
  }
            
  if(FlashStatus != HAL_OK) return false;
    
  pEraseInit.TypeErase = TYPEERASE_SECTORS;
  pEraseInit.Sector = old_page;
  pEraseInit.NbSectors = 1;
  pEraseInit.VoltageRange = VOLTAGE_RANGE;
  
  /* Erase the old Page: Set old Page status to ERASED status */
  if((FlashStatus = HAL_FLASHEx_Erase(&pEraseInit, &SectorError)) != HAL_OK) return false;  
  
  if((FlashStatus = HAL_FLASH_Program(TYPEPROGRAM_HALFWORD, NewPageAddress, VALID_PAGE)) != HAL_OK) return false;
  
  nv_store.page = new_page;
  nv_store.base = NewPageAddress;
  nv_store.end = NewPageAddress + (PAGE_SIZE - 1);
  
  ssLoggingPrint(ESsLoggingLevel_Debug, LOGGING_NO_CATEGORY, "NV pages switched");
  
  return true;
}


/**
  * @brief  Verify if specified page is fully erased.
  * @param  Address: page address
  *   This parameter can be one of the following values:
  *     @arg PAGE0_BASE_ADDRESS: Page0 base address
  *     @arg PAGE1_BASE_ADDRESS: Page1 base address
  * @retval page fully erased status:
  *           - 0: if Page not erased
  *           - 1: if Page erased
  */
static bool nv_is_page_erased(uint32_t Address)
{
  return true;
}


/**
  * @}
  */ 

/******************* (C) COPYRIGHT 2017 STMicroelectronics *****END OF FILE****/
