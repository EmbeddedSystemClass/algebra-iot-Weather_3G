/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SS_MT_API_H
#define _SS_MT_API_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
   
#define MT_FRAME_SOF              0xFE
#define MT_FRAME_SOF_SIZE         1
#define MT_FRAME_LEN_SIZE         1
#define MT_FRAME_CMD_SIZE         2
#define MT_FRAME_CMD_HEADER_SIZE  (MT_FRAME_LEN_SIZE + MT_FRAME_CMD_SIZE)
#define MT_FRAME_FCS_SIZE         1

/* Maximum length of data in the general frame format. The upper limit is 255 because of the
 * 1-byte length protocol. But the operation limit is lower for code size and ram savings so that
 * the uart driver can use 256 byte rx/tx queues and so
 * (MT_RPC_DATA_MAX + MT_RPC_FRAME_HDR_SZ + MT_UART_FRAME_OVHD) < 256
 */
#define MT_DATA_MAX       250

/* The 3 MSB's of the 1st command field byte are for command type. */
#define MT_CMD_TYPE_MASK  0xE0


/* The 5 LSB's of the 1st command field byte are for the subsystem. */
#define MT_SUBSYSTEM_MASK 0x1F
      
/* Error codes */
#define MT_SUCCESS        0     /* success */
#define MT_ERR_FAIL       1     /* no response */
#define MT_ERR_SUBSYSTEM  2     /* invalid subsystem */
#define MT_ERR_COMMAND_ID 3     /* invalid command ID */
#define MT_ERR_PARAMETER  4     /* invalid parameter */
#define MT_ERR_LENGTH     5     /* invalid length */
   
/* position of fields in the general format frame */
#define MT_FRAME_SOF_POS      0
#define MT_FRAME_CMD_POS      1
#define MT_CMD_LEN_POS        0
#define MT_CMD_CMD0_POS       1
#define MT_CMD_CMD1_POS       2
#define MT_CMD_DATA_POS       3
  
#define MT_CMD_HEADER_FILL(cmd, dataLen, cmdType, cmdId) \
do \
{ \
  cmd[MT_CMD_LEN_POS] = dataLen; \
  cmd[MT_CMD_CMD0_POS] = cmdType; \
  cmd[MT_CMD_CMD1_POS] = cmdId; \
} while(0)

#define MT_CMD_FRAME_LENGTH_GET(cmd)          (cmd[MT_CMD_LEN_POS] + MT_FRAME_CMD_SIZE + MT_FRAME_LEN_SIZE)
#define MT_CMD_DATA_LENGTH_GET(cmd)           (cmd[MT_CMD_LEN_POS])
#define MT_CMD_DATA_LENGTH_SET(cmd, length)   (cmd[MT_CMD_LEN_POS] = (uint8_t)length)
#define MT_CMD_TYPE_GET(cmd)                  (cmd[MT_CMD_CMD0_POS] & MT_CMD_TYPE_MASK)
#define MT_CMD_TYPE_SET(cmd, type)            (cmd[MT_CMD_CMD0_POS] = ((uint8_t)type & MT_CMD_TYPE_MASK) | (cmd[MT_CMD_CMD0_POS] & MT_CMD_TYPE_MASK))
#define MT_CMD_SUBSYSTEM_GET(cmd)             (cmd[MT_CMD_CMD0_POS] & MT_SUBSYSTEM_MASK)
#define MT_CMD_SUBSYSTEM_SET(cmd, subsystem)  (cmd[MT_CMD_CMD0_POS] = ((uint8_t)subsystem & MT_SUBSYSTEM_MASK) | (cmd[MT_CMD_CMD0_POS] & MT_SUBSYSTEM_MASK))
#define MT_CMD_TYPE_SUBSYSTEM_SET(cmd, type, subsystem) (cmd[MT_CMD_CMD0_POS] = (uint8_t)type | (uint8_t)subsystem)
#define MT_CMD_ID_GET(cmd)                    (cmd[MT_CMD_CMD1_POS])
#define MT_CMD_ID_SET(cmd, id)                (cmd[MT_CMD_CMD1_POS] = (uint8_t)id)
#define MT_CMD_DATA_PTR_GET(cmd)              (&cmd[MT_CMD_DATA_POS])
  
#define MTAPI_SEND_CMD_REQ_MSG_ID               0x1000
#define MTAPI_SEND_CMD_RESP_MSG_ID              0x1001
  
/***************************************************************************************************
 * MT COMMANDS
 ***************************************************************************************************/

#define MT_AREQ_ID  0x80

/***************************************************************************************************
 * SYS COMMANDS
 ***************************************************************************************************/
   
/* AREQ to RFM */
#define MT_SYS_RESET_REQ                     0x00

/* SREQ/SRSP */
#define MT_SYS_PING                          0x01
#define MT_SYS_PING_LENGTH                   2
#define MT_SYS_VERSION                       0x02
#define MT_SYS_SET_EXTADDR                   0x03
#define MT_SYS_GET_EXTADDR                   0x04
#define MT_SYS_RAM_READ                      0x05
#define MT_SYS_RAM_WRITE                     0x06
#define MT_SYS_OSAL_NV_ITEM_INIT             0x07
#define MT_SYS_OSAL_NV_READ                  0x08
#define MT_SYS_OSAL_NV_WRITE                 0x09
#define MT_SYS_OSAL_START_TIMER              0x0A
#define MT_SYS_OSAL_STOP_TIMER               0x0B
#define MT_SYS_RANDOM                        0x0C
#define MT_SYS_ADC_READ                      0x0D
#define MT_SYS_GPIO                          0x0E
#define MT_SYS_STACK_TUNE                    0x0F
#define MT_SYS_SET_TIME                      0x10
#define MT_SYS_GET_TIME                      0x11
#define MT_SYS_OSAL_NV_DELETE                0x12
#define MT_SYS_OSAL_NV_LENGTH                0x13
#define MT_SYS_SET_TX_POWER                  0x14
#define MT_SYS_JAMMER_PARAMETERS             0x15
#define MT_SYS_SNIFFER_PARAMETERS            0x16
#define MT_SYS_ZDIAGS_INIT_STATS             0x17
#define MT_SYS_ZDIAGS_CLEAR_STATS            0x18
#define MT_SYS_ZDIAGS_GET_STATS              0x19
#define MT_SYS_ZDIAGS_RESTORE_STATS_NV       0x1A
#define MT_SYS_ZDIAGS_SAVE_STATS_TO_NV       0x1B
#define MT_SYS_OSAL_NV_READ_EXT              0x1C
#define MT_SYS_OSAL_NV_WRITE_EXT             0x1D

/* AREQ from RFM */
#define MT_SYS_RESET_IND                     0x80
#define MT_SYS_RESET_IND_SIZE                6   
#define MT_SYS_OSAL_TIMER_EXPIRED            0x81
#define MT_SYS_JAMMER_IND                    0x82

#define MT_SYS_RESET_HARD     0
#define MT_SYS_RESET_SOFT     1
#define MT_SYS_RESET_SHUTDOWN 2

#define MT_SYS_SNIFFER_DISABLE       0
#define MT_SYS_SNIFFER_ENABLE        1
#define MT_SYS_SNIFFER_GET_SETTING   2

   
/***************************************************************************************************
 * UTIL COMMANDS
 ***************************************************************************************************/

/* SREQ/SRSP: */
#define MT_UTIL_GET_DEVICE_INFO              0x00
#define MT_UTIL_GET_NV_INFO                  0x01
#define MT_UTIL_SET_PANID                    0x02
#define MT_UTIL_SET_CHANNELS                 0x03
#define MT_UTIL_SET_SECLEVEL                 0x04
#define MT_UTIL_SET_PRECFGKEY                0x05
#define MT_UTIL_CALLBACK_SUB_CMD             0x06
#define MT_UTIL_KEY_EVENT                    0x07
#define MT_UTIL_TIME_ALIVE                   0x09
#define MT_UTIL_LED_CONTROL                  0x0A

#define MT_UTIL_TEST_LOOPBACK                0x10
#define MT_UTIL_DATA_REQ                     0x11

#define MT_UTIL_GPIO_SET_DIRECTION           0x14
#define MT_UTIL_GPIO_READ                    0x15
#define MT_UTIL_GPIO_WRITE                   0x16

#define MT_UTIL_SRC_MATCH_ENABLE             0x20
#define MT_UTIL_SRC_MATCH_ADD_ENTRY          0x21
#define MT_UTIL_SRC_MATCH_DEL_ENTRY          0x22
#define MT_UTIL_SRC_MATCH_CHECK_SRC_ADDR     0x23
#define MT_UTIL_SRC_MATCH_ACK_ALL_PENDING    0x24
#define MT_UTIL_SRC_MATCH_CHECK_ALL_PENDING  0x25

#define MT_UTIL_ADDRMGR_EXT_ADDR_LOOKUP      0x40
#define MT_UTIL_ADDRMGR_NWK_ADDR_LOOKUP      0x41
#define MT_UTIL_APSME_LINK_KEY_DATA_GET      0x44
#define MT_UTIL_APSME_LINK_KEY_NV_ID_GET     0x45
#define MT_UTIL_ASSOC_COUNT                  0x48
#define MT_UTIL_ASSOC_FIND_DEVICE            0x49
#define MT_UTIL_ASSOC_GET_WITH_ADDRESS       0x4A
#define MT_UTIL_APSME_REQUEST_KEY_CMD        0x4B
#define MT_UTIL_SRNG_GENERATE                0x4C
#define MT_UTIL_BIND_ADD_ENTRY               0x4D

#define MT_UTIL_ZCL_KEY_EST_INIT_EST         0x80
#define MT_UTIL_ZCL_KEY_EST_SIGN             0x81

/* AREQ from/to host */
#define MT_UTIL_SYNC_REQ                     0xE0
#define MT_UTIL_ZCL_KEY_ESTABLISH_IND        0xE1
  
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
/* command types */
typedef enum {
  MT_CMD_TYPE_POLL = 0x00,
  MT_CMD_TYPE_SREQ = 0x20,
  MT_CMD_TYPE_AREQ = 0x40,
  MT_CMD_TYPE_SRSP = 0x60,
  MT_CMD_TYPE_RES4 = 0x80,
  MT_CMD_TYPE_RES5 = 0xA0,
  MT_CMD_TYPE_RES6 = 0xC0,
  MT_CMD_TYPE_RES7 = 0xE0
} MtApiCmdTypeEnum;

/* MT Api subsystems */
typedef enum {
  MT_SUBSYSTEM_RES0,   /* Reserved. */
  MT_SUBSYSTEM_SYS,
  MT_SUBSYSTEM_MAC,
  MT_SUBSYSTEM_NWK,
  MT_SUBSYSTEM_AF,
  MT_SUBSYSTEM_ZDO,
  MT_SUBSYSTEM_SAPI,   /* Simple API. */
  MT_SUBSYSTEM_UTIL,
  MT_SUBSYSTEM_DBG,
  MT_SUBSYSTEM_APP,
  MT_SUBSYSTEM_OTA,
  MT_SUBSYSTEM_ZNP,
  MT_SUBSYSTEM_SPARE_12,
  MT_SUBSYSTEM_UBL = 13,  // 13 to be compatible with existing RemoTI.
  MT_SUBSYSTEM_RES14,
  MT_SUBSYSTEM_RES15,
  MT_SUBSYSTEM_RES16,
  MT_SUBSYSTEM_PROTOBUF,
  MT_SUBSYSTEM_RES18,  // RPC_SYS_PB_NWK_MGR
  MT_SUBSYSTEM_RES19,  // RPC_SYS_PB_GW
  MT_SUBSYSTEM_RES20,  // RPC_SYS_PB_OTA_MGR
  MT_SUBSYSTEM_GP = 21,
  MT_SUBSYSTEM_USER_APP,
  MT_SUBSYSTEM_MAX     /* Maximum value, must be last */
  /* 22-32 available, not yet assigned. */
} MtApiSubsystemTypeEnum;
  
  
typedef struct
{
  uint8_t len;
  uint8_t cmd0;
  uint8_t cmd1;
  uint8_t data[0];
} MtApiCmdType;

typedef struct
{
  uint8_t status;
  MtApiCmdType cmd[0];
} MtApiCmdRespType;

typedef uint8_t (*MtApiSysCbk)(uint8_t cmdId, uint8_t dataLen, uint8_t *pData);
  
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
void ssMtApiInit(void);

uint32_t MtApiRegisterSubsystem(MtApiSubsystemTypeEnum subsystem, MtApiSysCbk callback);

void MtApiCmdPrepare(uint8_t *cmd, uint8_t cmdType, uint8_t cmdId, uint8_t dataLen);
void MtApiCmdSend(uint8_t cmdType, uint8_t cmdId, uint8_t dataLen, uint8_t *pData);
uint8_t *MtApiCmdSendX(uint8_t *request);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _SS_MT_API_H */
 