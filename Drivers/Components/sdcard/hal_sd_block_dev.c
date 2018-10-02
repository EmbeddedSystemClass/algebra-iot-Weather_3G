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

#include <stdint.h>
#include <stdbool.h>

#include "stm32f4xx_hal.h"

#include "bsp.h"

#include "hal_sd_block_dev.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

typedef struct hal_sd_block_dev_t
{
  SPI_HandleTypeDef *hspi;
} hal_sd_block_dev_t;

/*------------------------- PRIVATE VARIABLES --------------------------------*/

/** GO_IDLE_STATE - init card in spi mode if CS low */
static const uint8_t CMD0 = 0X00;
/** SEND_IF_COND - verify SD Memory Card interface operating condition.*/
static const uint8_t CMD8 = 0X08;
/** SEND_CSD - read the Card Specific Data (CSD register) */
//static const uint8_t CMD9 = 0X09;
/** SEND_CID - read the card identification information (CID register) */
//static const uint8_t CMD10 = 0X0A;
/** SEND_STATUS - read the card status register */
//static const uint8_t CMD13 = 0X0D;
/** READ_BLOCK - read a single data block from the card */
//static const uint8_t CMD17 = 0X11;
/** WRITE_BLOCK - write a single data block to the card */
//static const uint8_t CMD24 = 0X18;
/** WRITE_MULTIPLE_BLOCK - write blocks of data until a STOP_TRANSMISSION */
//static const uint8_t CMD25 = 0X19;
/** ERASE_WR_BLK_START - sets the address of the first block to be erased */
//static const uint8_t CMD32 = 0X20;
/** ERASE_WR_BLK_END - sets the address of the last block of the continuous
 range to be erased*/
//static const uint8_t CMD33 = 0X21;
/** ERASE - erase all previously selected blocks */
//static const uint8_t CMD38 = 0X26;
/** APP_CMD - escape for application specific command */
static const uint8_t CMD55 = 0X37;
/** READ_OCR - read the OCR register of a card */
static const uint8_t CMD58 = 0X3A;
/** SET_WR_BLK_ERASE_COUNT - Set the number of write blocks to be
 pre-erased before writing */
//static const uint8_t ACMD23 = 0X17;
/** SD_SEND_OP_COMD - Sends host capacity support information and
 activates the card's initialization process */
static const uint8_t ACMD41 = 0X29;
//------------------------------------------------------------------------------
/** status for card in the ready state */
static const uint8_t R1_READY_STATE = 0X00;
/** status for card in the idle state */
static const uint8_t R1_IDLE_STATE = 0X01;
/** status bit for illegal command */
static const uint8_t R1_ILLEGAL_COMMAND = 0X04;
/** start data token for read or write single block*/
//static const uint8_t DATA_START_BLOCK = 0XFE;
/** stop token for write multiple blocks*/
//static const uint8_t STOP_TRAN_TOKEN = 0XFD;
/** start data token for write multiple blocks*/
//static const uint8_t WRITE_MULTIPLE_TOKEN = 0XFC;
/** mask for data response tokens after a write block operation */
//static const uint8_t DATA_RES_MASK = 0X1F;
/** write data accepted token */
//static const uint8_t DATA_RES_ACCEPTED = 0X05;

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/

hal_sd_block_dev_t sd_card_dev;

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

static bool _init(void);
static void _select(void);
static void _deselect(void);
static HAL_StatusTypeDef _receive(hal_sd_block_dev_t *hdev, uint8_t* pData, uint16_t Size);
static uint8_t _cmd(hal_sd_block_dev_t *hdev, uint8_t command, uint32_t arg);
static uint8_t _acmd(hal_sd_block_dev_t *hdev, uint8_t cmd, uint32_t arg);
static void _wait_ready(void);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

hal_sd_blk_dev_handle_t hal_sd_block_dev_init(void)
{
  hal_sd_blk_dev_handle_t hdev = NULL;
  if(_init())
  {
    sd_card_dev.hspi = &bsp_sd_spi_handle;
    hdev = (hal_sd_blk_dev_handle_t) &sd_card_dev;
  }
  
  return hdev;
}


hal_sd_status_e hal_sd_block_dev_read(hal_sd_blk_dev_handle_t hsd, uint32_t *dest, uint64_t address, uint32_t block_size, uint32_t block_cnt)
{
  return SD_OK;
}


hal_sd_status_e hal_sd_block_dev_write(hal_sd_blk_dev_handle_t hsd, uint32_t *src, uint64_t address, uint32_t block_size, uint32_t block_cnt)
{
  return SD_OK;
}


hal_sd_status_e hal_sd_block_dev_erase(hal_sd_blk_dev_handle_t hsd, uint64_t address, uint64_t size)
{
  return SD_OK;
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

bool _init()
{	
  bsp_sd_spi_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256; //slow down at first
	HAL_SPI_Init(&bsp_sd_spi_handle); //apply the speed change
	_deselect();
//We must supply at least 74 clocks with CS high
	uint8_t buffer[4] = { 0xFF, 0xFF, 0xFF, 0xFF };
	HAL_SPI_Transmit(&bsp_sd_spi_handle, buffer, 4, 100);
	HAL_SPI_Transmit(&bsp_sd_spi_handle, buffer, 4, 100);
	HAL_SPI_Transmit(&bsp_sd_spi_handle, buffer, 4, 100);
	HAL_Delay(5);
	_select();

	// command to go idle in SPI mode
	while (_cmd(&sd_card_dev, CMD0, 0) != R1_IDLE_STATE);
  
	// check SD version
	if ((_cmd(&sd_card_dev, CMD8, 0x1AA) & R1_ILLEGAL_COMMAND)) {
		_deselect();
		return false; //Unsupported
	} else {
		// only need last byte of r7 response
		HAL_SPI_Receive(&bsp_sd_spi_handle, buffer, 4, 100);
		if (buffer[3] != 0XAA) {
			return false; //failed check
		}

	}
	// initialize card and send host supports SDHC
	while (_acmd(&sd_card_dev, ACMD41, 0X40000000) != R1_READY_STATE) {

	}
	// if SD2 read OCR register to check for SDHC card
	if (_cmd(&sd_card_dev, CMD58, 0)) {
		_deselect();
		return false;
	}
	//discard OCR reg

	_receive(&sd_card_dev, buffer, 4);
	_deselect();
	bsp_sd_spi_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4; //speed back up
	HAL_SPI_Init(&bsp_sd_spi_handle); //apply the speed change
  
  return true;
}

void _select() 
{
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);
}

void _deselect() 
{
	HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);
}


HAL_StatusTypeDef _receive(hal_sd_block_dev_t *hdev, uint8_t* pData, uint16_t Size) 
{
	HAL_StatusTypeDef errorcode = HAL_OK;

	/* Process Locked */
	__HAL_LOCK(hdev->hspi);

	/* Don't overwrite in case of HAL_SPI_STATE_BUSY_RX */
	if (hdev->hspi->State == HAL_SPI_STATE_READY) {
		hdev->hspi->State = HAL_SPI_STATE_BUSY_TX_RX;
	}

	/* Set the transaction information */
	hdev->hspi->ErrorCode = HAL_SPI_ERROR_NONE;
	hdev->hspi->pRxBuffPtr = (uint8_t *) pData;
	hdev->hspi->RxXferCount = Size;
	hdev->hspi->RxXferSize = Size;
	hdev->hspi->pTxBuffPtr = (uint8_t *) pData;
	hdev->hspi->TxXferCount = Size;
	hdev->hspi->TxXferSize = Size;

	/*Init field not used in handle to zero */
	hdev->hspi->RxISR = NULL;
	hdev->hspi->TxISR = NULL;
	/* Check if the SPI is already enabled */
	if ((hdev->hspi->Instance->CR1 & SPI_CR1_SPE) != SPI_CR1_SPE) {
		/* Enable SPI peripheral */
		__HAL_SPI_ENABLE(hdev->hspi);
	}
	/* Transmit and Receive data in 8 Bit mode */
	while ((hdev->hspi->RxXferCount > 0U)) {
		*(__IO uint8_t *) &hdev->hspi->Instance->DR = 0xFF; //send data
		while (!(__HAL_SPI_GET_FLAG(hdev->hspi, SPI_FLAG_TXE)))
			;
		while (!(__HAL_SPI_GET_FLAG(hdev->hspi, SPI_FLAG_RXNE)))
			;
		(*(uint8_t *) pData++) = hdev->hspi->Instance->DR;
		hdev->hspi->RxXferCount--;
	}

	if (lSPI_WaitFlagStateUntilTimeout(hdev->hspi, SPI_FLAG_BSY, RESET, 100,
			HAL_GetTick()) != HAL_OK) {
		hdev->hspi->ErrorCode |= HAL_SPI_ERROR_FLAG;

		errorcode = HAL_TIMEOUT;
	}

	hdev->hspi->State = HAL_SPI_STATE_READY;
	__HAL_UNLOCK(hdev->hspi);
	return errorcode;
}


void _wait_ready() 
{
	uint8_t ans[1] = { 0 };
	while (ans[0] != 0xFF) 
  {
		_receive(&sd_card_dev, ans, 1);
	}
}

uint8_t _cmd(hal_sd_block_dev_t *hdev, uint8_t command, uint32_t arg) 
{
	uint8_t res = 0xFF;
	/*HAL_SPI_Transmit(hdev->hspi, &res, 1, 100);
	 HAL_SPI_Transmit(hdev->hspi, &res, 1, 100);
	 HAL_SPI_Transmit(hdev->hspi, &res, 1, 100);
	 HAL_SPI_Transmit(hdev->hspi, &res, 1, 100);
	 HAL_SPI_Transmit(hdev->hspi, &res, 1, 100);
	 HAL_SPI_Transmit(hdev->hspi, &res, 1, 100);
	 HAL_SPI_Transmit(hdev->hspi, &res, 1, 100);*/

	_select();
	_wait_ready(); //wait for card to no longer be busy
	uint8_t commandSequence[] = { (uint8_t) (command | 0x40), (uint8_t) (arg
			>> 24), (uint8_t) (arg >> 16), (uint8_t) (arg >> 8), (uint8_t) (arg
			& 0xFF), 0xFF };
	if (command == CMD0)
		commandSequence[5] = 0x95;
	else if (command == CMD8)
		commandSequence[5] = 0x87;
	HAL_SPI_Transmit(hdev->hspi, commandSequence, 6, 100);
	//Data sent, now await Response
	uint8_t count = 20;
	while ((res & 0x80) && count) {
		_receive(hdev, &res, 1);
		count--;
	}
	return res;
}

uint8_t _acmd(hal_sd_block_dev_t *hdev, uint8_t cmd, uint32_t arg) 
{
		_cmd(hdev, CMD55, 0);
		return _cmd(hdev, cmd, arg);
}
