/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#if (!defined _HAL_SD_BLOCK_DEV_H)
#define _HAL_SD_BLOCK_DEV_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/**
  * @brief Supported SD Memory Cards
  */
#define STD_CAPACITY_SD_CARD_V1_1             ((uint32_t)0x00000000)
#define STD_CAPACITY_SD_CARD_V2_0             ((uint32_t)0x00000001)
#define HIGH_CAPACITY_SD_CARD                 ((uint32_t)0x00000002)
#define MULTIMEDIA_CARD                       ((uint32_t)0x00000003)
#define SECURE_DIGITAL_IO_CARD                ((uint32_t)0x00000004)
#define HIGH_SPEED_MULTIMEDIA_CARD            ((uint32_t)0x00000005)
#define SECURE_DIGITAL_IO_COMBO_CARD          ((uint32_t)0x00000006)
#define HIGH_CAPACITY_MMC_CARD                ((uint32_t)0x00000007)

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

typedef void* hal_sd_blk_dev_handle_t;


/** @defgroup SD_Exported_Types_Group2 Card Specific Data: CSD Register
  * @{
  */
typedef struct
{
  __IO uint8_t  CSDStruct;            /*!< CSD structure                         */
  __IO uint8_t  SysSpecVersion;       /*!< System specification version          */
  __IO uint8_t  Reserved1;            /*!< Reserved                              */
  __IO uint8_t  TAAC;                 /*!< Data read access time 1               */
  __IO uint8_t  NSAC;                 /*!< Data read access time 2 in CLK cycles */
  __IO uint8_t  MaxBusClkFrec;        /*!< Max. bus clock frequency              */
  __IO uint16_t CardComdClasses;      /*!< Card command classes                  */
  __IO uint8_t  RdBlockLen;           /*!< Max. read data block length           */
  __IO uint8_t  PartBlockRead;        /*!< Partial blocks for read allowed       */
  __IO uint8_t  WrBlockMisalign;      /*!< Write block misalignment              */
  __IO uint8_t  RdBlockMisalign;      /*!< Read block misalignment               */
  __IO uint8_t  DSRImpl;              /*!< DSR implemented                       */
  __IO uint8_t  Reserved2;            /*!< Reserved                              */
  __IO uint32_t DeviceSize;           /*!< Device Size                           */
  __IO uint8_t  MaxRdCurrentVDDMin;   /*!< Max. read current @ VDD min           */
  __IO uint8_t  MaxRdCurrentVDDMax;   /*!< Max. read current @ VDD max           */
  __IO uint8_t  MaxWrCurrentVDDMin;   /*!< Max. write current @ VDD min          */
  __IO uint8_t  MaxWrCurrentVDDMax;   /*!< Max. write current @ VDD max          */
  __IO uint8_t  DeviceSizeMul;        /*!< Device size multiplier                */
  __IO uint8_t  EraseGrSize;          /*!< Erase group size                      */
  __IO uint8_t  EraseGrMul;           /*!< Erase group size multiplier           */
  __IO uint8_t  WrProtectGrSize;      /*!< Write protect group size              */
  __IO uint8_t  WrProtectGrEnable;    /*!< Write protect group enable            */
  __IO uint8_t  ManDeflECC;           /*!< Manufacturer default ECC              */
  __IO uint8_t  WrSpeedFact;          /*!< Write speed factor                    */
  __IO uint8_t  MaxWrBlockLen;        /*!< Max. write data block length          */
  __IO uint8_t  WriteBlockPaPartial;  /*!< Partial blocks for write allowed      */
  __IO uint8_t  Reserved3;            /*!< Reserved                              */
  __IO uint8_t  ContentProtectAppli;  /*!< Content protection application        */
  __IO uint8_t  FileFormatGrouop;     /*!< File format group                     */
  __IO uint8_t  CopyFlag;             /*!< Copy flag (OTP)                       */
  __IO uint8_t  PermWrProtect;        /*!< Permanent write protection            */
  __IO uint8_t  TempWrProtect;        /*!< Temporary write protection            */
  __IO uint8_t  FileFormat;           /*!< File format                           */
  __IO uint8_t  ECC;                  /*!< ECC code                              */
  __IO uint8_t  CSD_CRC;              /*!< CSD CRC                               */
  __IO uint8_t  Reserved4;            /*!< Always 1                              */

} hal_sd_csd_t;
/**
  * @}
  */

/** @defgroup SD_Exported_Types_Group3 Card Identification Data: CID Register
  * @{
  */
typedef struct
{
  __IO uint8_t  ManufacturerID;  /*!< Manufacturer ID       */
  __IO uint16_t OEM_AppliID;     /*!< OEM/Application ID    */
  __IO uint32_t ProdName1;       /*!< Product Name part1    */
  __IO uint8_t  ProdName2;       /*!< Product Name part2    */
  __IO uint8_t  ProdRev;         /*!< Product Revision      */
  __IO uint32_t ProdSN;          /*!< Product Serial Number */
  __IO uint8_t  Reserved1;       /*!< Reserved1             */
  __IO uint16_t ManufactDate;    /*!< Manufacturing Date    */
  __IO uint8_t  CID_CRC;         /*!< CID CRC               */
  __IO uint8_t  Reserved2;       /*!< Always 1              */

} hal_sd_cid_t;
/**
  * @}
  */

/** @defgroup SD_Exported_Types_Group4 SD Card Status returned by ACMD13
  * @{
  */
typedef struct
{
  __IO uint8_t  DAT_BUS_WIDTH;           /*!< Shows the currently defined data bus width                 */
  __IO uint8_t  SECURED_MODE;            /*!< Card is in secured mode of operation                       */
  __IO uint16_t SD_CARD_TYPE;            /*!< Carries information about card type                        */
  __IO uint32_t SIZE_OF_PROTECTED_AREA;  /*!< Carries information about the capacity of protected area   */
  __IO uint8_t  SPEED_CLASS;             /*!< Carries information about the speed class of the card      */
  __IO uint8_t  PERFORMANCE_MOVE;        /*!< Carries information about the card's performance move      */
  __IO uint8_t  AU_SIZE;                 /*!< Carries information about the card's allocation unit size  */
  __IO uint16_t ERASE_SIZE;              /*!< Determines the number of AUs to be erased in one operation */
  __IO uint8_t  ERASE_TIMEOUT;           /*!< Determines the timeout for any number of AU erase          */
  __IO uint8_t  ERASE_OFFSET;            /*!< Carries information about the erase offset                 */

}HAL_SD_CardStatusTypedef;
/**
  * @}
  */

/** @defgroup SD_Exported_Types_Group5 SD Card information structure
  * @{
  */
typedef struct
{
  hal_sd_csd_t        SD_csd;         /*!< SD card specific data register         */
  hal_sd_cid_t        SD_cid;         /*!< SD card identification number register */
  uint64_t            CardCapacity;   /*!< Card capacity                          */
  uint32_t            CardBlockSize;  /*!< Card block size                        */
  uint16_t            RCA;            /*!< SD relative card address               */
  uint8_t             CardType;       /*!< SD card type                           */

} hal_sd_card_info_t;

/** @defgroup SD_Exported_Types_Group6 SD Error status enumeration Structure definition
  * @{
  */
typedef enum
{
  SD_OK                              = (0),
/**
  * @brief  SD specific error defines
  */
  SD_CMD_CRC_FAIL                    = (1),   /*!< Command response received (but CRC check failed)              */
  SD_DATA_CRC_FAIL                   = (2),   /*!< Data block sent/received (CRC check failed)                   */
  SD_CMD_RSP_TIMEOUT                 = (3),   /*!< Command response timeout                                      */
  SD_DATA_TIMEOUT                    = (4),   /*!< Data timeout                                                  */
  SD_TX_UNDERRUN                     = (5),   /*!< Transmit FIFO underrun                                        */
  SD_RX_OVERRUN                      = (6),   /*!< Receive FIFO overrun                                          */
  SD_START_BIT_ERR                   = (7),   /*!< Start bit not detected on all data signals in wide bus mode   */
  SD_CMD_OUT_OF_RANGE                = (8),   /*!< Command's argument was out of range.                          */
  SD_ADDR_MISALIGNED                 = (9),   /*!< Misaligned address                                            */
  SD_BLOCK_LEN_ERR                   = (10),  /*!< Transferred block length is not allowed for the card or the number of transferred bytes does not match the block length */
  SD_ERASE_SEQ_ERR                   = (11),  /*!< An error in the sequence of erase command occurs.            */
  SD_BAD_ERASE_PARAM                 = (12),  /*!< An invalid selection for erase groups                        */
  SD_WRITE_PROT_VIOLATION            = (13),  /*!< Attempt to program a write protect block                     */
  SD_LOCK_UNLOCK_FAILED              = (14),  /*!< Sequence or password error has been detected in unlock command or if there was an attempt to access a locked card */
  SD_COM_CRC_FAILED                  = (15),  /*!< CRC check of the previous command failed                     */
  SD_ILLEGAL_CMD                     = (16),  /*!< Command is not legal for the card state                      */
  SD_CARD_ECC_FAILED                 = (17),  /*!< Card internal ECC was applied but failed to correct the data */
  SD_CC_ERROR                        = (18),  /*!< Internal card controller error                               */
  SD_GENERAL_UNKNOWN_ERROR           = (19),  /*!< General or unknown error                                     */
  SD_STREAM_READ_UNDERRUN            = (20),  /*!< The card could not sustain data transfer in stream read operation. */
  SD_STREAM_WRITE_OVERRUN            = (21),  /*!< The card could not sustain data programming in stream mode   */
  SD_CID_CSD_OVERWRITE               = (22),  /*!< CID/CSD overwrite error                                      */
  SD_WP_ERASE_SKIP                   = (23),  /*!< Only partial address space was erased                        */
  SD_CARD_ECC_DISABLED               = (24),  /*!< Command has been executed without using internal ECC         */
  SD_ERASE_RESET                     = (25),  /*!< Erase sequence was cleared before executing because an out of erase sequence command was received */
  SD_AKE_SEQ_ERROR                   = (26),  /*!< Error in sequence of authentication.                         */
  SD_INVALID_VOLTRANGE               = (27),
  SD_ADDR_OUT_OF_RANGE               = (28),
  SD_SWITCH_ERROR                    = (29),
  SD_SDIO_DISABLED                   = (30),
  SD_SDIO_FUNCTION_BUSY              = (31),
  SD_SDIO_FUNCTION_FAILED            = (32),
  SD_SDIO_UNKNOWN_FUNCTION           = (33),

/**
  * @brief  Standard error defines
  */
  SD_INTERNAL_ERROR                  = (34),
  SD_NOT_CONFIGURED                  = (35),
  SD_REQUEST_PENDING                 = (36),
  SD_REQUEST_NOT_APPLICABLE          = (37),
  SD_INVALID_PARAMETER               = (38),
  SD_UNSUPPORTED_FEATURE             = (39),
  SD_UNSUPPORTED_HW                  = (40),
  SD_ERROR                           = (41),
} hal_sd_status_e;

/**
  * @}
  */

/** @defgroup SD_Exported_Types_Group7 SD Transfer state enumeration structure
  * @{
  */
typedef enum
{
  SD_TRANSFER_OK    = 0,  /*!< Transfer success      */
  SD_TRANSFER_BUSY  = 1,  /*!< Transfer is occurring */
  SD_TRANSFER_ERROR = 2   /*!< Transfer failed       */

} hal_sd_transfer_state_e;

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

hal_sd_blk_dev_handle_t hal_sd_block_dev_init(void);
hal_sd_status_e hal_sd_block_dev_read(hal_sd_blk_dev_handle_t hsd, uint32_t *dest, uint64_t address, uint32_t block_size, uint32_t block_cnt);
hal_sd_status_e hal_sd_block_dev_write(hal_sd_blk_dev_handle_t hsd, uint32_t *src, uint64_t address, uint32_t block_size, uint32_t block_cnt);
hal_sd_status_e hal_sd_block_dev_erase(hal_sd_blk_dev_handle_t hsd, uint64_t address, uint64_t size);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _HAL_SD_BLOCK_DEV_H */

