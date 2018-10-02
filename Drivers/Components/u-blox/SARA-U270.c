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
extern "C"
{
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "bsp.h"
#include "ssLogging.h"
#include "FreeRTOS.h"
#include "task.h"
#include "ssUart.h"
#include "SARA-U270.h"
#include "main.h"
#include "cmsis_os.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/
#if !defined(MODEM_UART)
#define MODEM_UART                	USART3
#endif
#if !defined(MODEM_UART_BAUDRATE)
#define MODEM_UART_BAUDRATE       	115200
#endif
#define MODEM_UART_WORDLENGTH		  UART_WORDLENGTH_8B
#define MODEM_UART_STOPBITS      	UART_STOPBITS_1
#define MODEM_UART_PARITY        	UART_PARITY_NONE
#define MODEM_UART_MODE          	UART_MODE_TX_RX
#define MODEM_UART_FLOWCONTROL   	UART_HWCONTROL_RTS_CTS
#define MODEM_UART_BUFFER_SIZE   	32

#define MODEM_POWER_ON_RETRY_MAX 10

#define MODEM_RX_BUF_SIZE        1024
#define MODEM_TX_BUF_SIZE        1024

#define S3_CHAR '\r'
#define S4_CHAR '\n'

#define MODEM_DEFAULT_TIMEOUT 10000

//! An IP v4 address
typedef uint32_t MDM_IP;
#define NOIP ((MDM_IP)0) //!< No IP address
// IP number formating and conversion
#define IPSTR           "%d.%d.%d.%d"
#define IPNUM(ip)       ((ip)>>24)&0xff, \
                        ((ip)>>16)&0xff, \
                        ((ip)>> 8)&0xff, \
                        ((ip)>> 0)&0xff
#define IPADR(a,b,c,d) ((((uint32_t)(a))<<24) | \
                        (((uint32_t)(b))<<16) | \
                        (((uint32_t)(c))<< 8) | \
                        (((uint32_t)(d))<< 0))

// num sockets
#define NUMSOCKETS 7

//! Socket error return codes
#define MDM_SOCKET_ERROR    (-1)

#define PROFILE         "0"   //!< this is the psd profile used

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

typedef enum EchoStatus_e
{
  Echo_Off = 0,
  Echo_On = 1
} EchoStatus_e;

typedef enum ErrorReportFormat_e
{
  ErrorReportFormat_None = 0,
  ErrorReportFormat_Number = 1,
  ErrorReportFormat_Verbose = 2,
} ErrorReportFormat_e;

typedef enum ResponseFormat_e
{
  ResponseFormat_Limited = 0,
  ResponseFormat_Full = 1,
} ResponseFormat_e;

typedef enum EnterPinCmdResponse_e
{
  EnterPinCmdResponse_Ready = 0,
  EnterPinCmdResponse_WaitPin = 1,
  EnterPinCmdResponse_WaitPuk = 2,
  EnterPinCmdResponse_WaitPin2 = 3,
  EnterPinCmdResponse_WaitPuk2 = 4,
  EnterPinCmdResponse_PhNetWait = 5,
  EnterPinCmdResponse_PhNetsubWait = 6,
  EnterPinCmdResponse_PhSpWait = 7,
  EnterPinCmdResponse_PhCorpWait = 8,
  EnterPinCmdResponse_PhSimWait = 9,
  EnterPinCmdResponse_NoSim = 10,
} EnterPinCmdResponse_e;

// management struture for sockets
typedef struct {
    int handle;
    uint32_t timeout_ms;
    volatile uint32_t connected;
    volatile int pending;
    volatile uint32_t open;
} SockCtrl;



/** callback function for
 \param result final result code
 \param response information text response
 \param len response length
 \param param the optional argument
 \return 0 if processing should continue, >0 in case of error
 */
typedef uint32_t (*CmdRespHandler_t)(int32_t type, const uint8_t* buf, uint32_t len, void* param);

typedef enum AtCmdResultType_e
{
  AtCmdResultType_Final = 0,
  AtCmdResultType_Intermediate = 1,
  AtCmdResultType_Unsolicited = 2,
  AtCmdResultType_Inf = 3,
  AtCmdResultType_Err = 4,
} AtCmdResultType_e;

typedef enum AtCmdResult_e
{
  AtCmdResult_OK = 0, /* (Final) Command line successfully processed
   and the command is correctly executed */
  AtCmdResult_CONNECT = 1, /* (Intermediate) Data connection established */
  AtCmdResult_RING = 2, /* (Unsolicited) Incoming call signal from the network */
  AtCmdResult_NO_CARRIER = 3, /* (Final) Connection terminated from the remote part or attempt to establish a connection failed */
  AtCmdResult_ERROR = 4, /* (Final) General failure. The AT+CMEE command configures the error result format */
  AtCmdResult_NO_DIALTONE = 6, /* (Final) No dialtone detected */
  AtCmdResult_BUSY = 7, /* (Final) Engaged signal detected (the called number is busy) */
  AtCmdResult_NO_ANSWER = 8, /* (Final) No hang up detected after a fixed network timeout */
} AtCmdResult_e;

typedef struct AtCmdResultCodeEntry_t
{
  char *verbose;
  AtCmdResult_e code;
  AtCmdResultType_e type;
} AtCmdResultCodeEntry_t;

typedef struct AtCmdResponseEntry_t
{
  char *response;
  uint32_t code;
} AtCmdResponseEntry_t;


enum {
    // waitFinalResp Responses
    NOT_FOUND     =  0,
    WAIT          = -1, // TIMEOUT
    RESP_OK       = -2,
    RESP_ERROR    = -3,
    RESP_PROMPT   = -4,
    RESP_ABORTED  = -5,

    // getLine Responses
    #define LENGTH(x)  (x & 0x00FFFF) //!< extract/mask the length
    #define TYPE(x)    (x & 0xFF0000) //!< extract/mask the type

    TYPE_UNKNOWN    = 0x000000,
    TYPE_OK         = 0x110000,
    TYPE_ERROR      = 0x120000,
    TYPE_RING       = 0x210000,
    TYPE_CONNECT    = 0x220000,
    TYPE_NOCARRIER  = 0x230000,
    TYPE_NODIALTONE = 0x240000,
    TYPE_BUSY       = 0x250000,
    TYPE_NOANSWER   = 0x260000,
    TYPE_PROMPT     = 0x300000,
    TYPE_PLUS       = 0x400000,
    TYPE_TEXT       = 0x500000,
    TYPE_ABORTED    = 0x600000,
    TYPE_DBLNEWLINE = 0x700000,

    // special timout constant
    TIMEOUT_BLOCKING = 0xffffffff
};

//! MT Device Types
typedef enum
{
  DEV_UNKNOWN,
  DEV_SARA_G350,
  DEV_LISA_U200,
  DEV_LISA_C200,
  DEV_SARA_U260,
  DEV_SARA_U270,
  DEV_LEON_G200
} Dev;

//! SIM Status
typedef enum
{
  SIM_UNKNOWN,
  SIM_MISSING,
  SIM_PIN,
  SIM_READY
} Sim;

//! SIM Status
typedef enum
{
  LPM_DISABLED,
  LPM_ENABLED,
  LPM_ACTIVE
} Lpm;

//! Device status
typedef struct {
    Dev dev;            //!< Device Type
    Lpm lpm;            //!< Power Saving
    Sim sim;            //!< SIM Card Status
    char ccid[20+1];    //!< Integrated Circuit Card ID
    char imsi[15+1];    //!< International Mobile Station Identity
    char imei[15+1];    //!< International Mobile Equipment Identity
    char meid[18+1];    //!< Mobile Equipment IDentifier
    char manu[16];      //!< Manufacturer (u-blox)
    char model[16];     //!< Model Name (LISA-U200, LISA-C200 or SARA-G350)
    char ver[16];       //!< Software Version
} DevStatus;

//! Registration Status
typedef enum
{
  REG_UNKNOWN,
  REG_DENIED,
  REG_NONE,
  REG_HOME,
  REG_ROAMING
} Reg;

//! Access Technology
typedef enum
{
  ACT_UNKNOWN,
  ACT_GSM,
  ACT_EDGE,
  ACT_UTRAN,
  ACT_CDMA
} AcT;

//! Network Status
typedef struct
{
  Reg csd;        //!< CSD Registration Status (Circuit Switched Data)
  Reg psd;        //!< PSD Registration status (Packet Switched Data)
  AcT act;        //!< Access Technology
  int rssi;       //!< Received Signal Strength Indication (in dBm, range -113..-53)
  int qual;       //!< In UMTS RAT indicates the Energy per Chip/Noise ratio in dB levels
                  //!< of the current cell (see <ecn0_ lev> in +CGED command description),
                  //!< see 3GPP TS 45.008 [20] subclause 8.2.4
  char opr[16+1]; //!< Operator Name
  char num[32];   //!< Mobile Directory Number
  unsigned short lac;  //!< location area code in hexadecimal format (2 bytes in hex)
  unsigned int ci;     //!< Cell ID in hexadecimal format (2 to 4 bytes in hex)
} NetStatus;

typedef struct
{
  uint16_t size;
  int cid;
  int tx_session;
  int rx_session;
  int tx_total;
  int rx_total;
} MDM_DataUsage;

typedef void (*_CELLULAR_SMS_CB)(void* data, int index);

typedef struct SARA_U270_Control_t
{
  int32_t huart;
  uint8_t *rx_buf;
  uint8_t *tx_buf;
  EchoStatus_e echo;
  ResponseFormat_e rspfmt;

  DevStatus   dev; //!< collected device information
  NetStatus   net; //!< collected network information
  MDM_IP      ip;  //!< assigned ip address
  MDM_DataUsage data_usage; //!< collected data usage information
  // Cellular callback to notify of new SMS
  _CELLULAR_SMS_CB sms_cb;
  void* sms_data;

  // LISA-C has 6 TCP and 6 UDP sockets
  // LISA-U and SARA-G have 7 sockets
  SockCtrl sockets[NUMSOCKETS];

  uint32_t activated;
  uint32_t attached;
  uint32_t attached_urc;
  volatile uint32_t cancel_all_operations;
} SARA_U270_Control_t;

/*------------------------- PUBLIC VARIABLES ---------------------------------*/



/*------------------------- PRIVATE VARIABLES --------------------------------*/

const AtCmdResultCodeEntry_t AtCmdResultCodeTable[] =
{
  { "OK",           AtCmdResult_OK,           AtCmdResultType_Final},
  { "CONNECT",      AtCmdResult_CONNECT,      AtCmdResultType_Intermediate},
  { "RING",         AtCmdResult_RING,         AtCmdResultType_Unsolicited},
  { "NO CARRIER",   AtCmdResult_NO_CARRIER,   AtCmdResultType_Final},
  { "ERROR",        AtCmdResult_ERROR,        AtCmdResultType_Final},
  { "NO DIALTONE",  AtCmdResult_NO_DIALTONE,  AtCmdResultType_Final},
  { "BUSY",         AtCmdResult_BUSY,         AtCmdResultType_Final},
  { "NO ANSWER",    AtCmdResult_NO_ANSWER,    AtCmdResultType_Final},
  { "+CME ERROR:",  AtCmdResult_ERROR,        AtCmdResultType_Final}
};

const AtCmdResultCodeEntry_t EnterPinCmdResponseTable[] =
{
  {"READY",         EnterPinCmdResponse_Ready,        AtCmdResultType_Inf}, /* MT is not pending for any password */
  {"SIM PIN",       EnterPinCmdResponse_WaitPin,      AtCmdResultType_Inf}, /* MT is waiting SIM PIN to be given */
#if 0
  {"SIM PUK",       EnterPinCmdResponse_WaitPuk,      AtCmdResultType_Inf}, /* MT is waiting SIM PUK to be given */
  {"SIM PIN2",      EnterPinCmdResponse_WaitPin2,     AtCmdResultType_Inf}, /* MT is waiting SIM PIN2 to be given */
  {"SIM PUK2",      EnterPinCmdResponse_WaitPuk2,     AtCmdResultType_Inf}, /* MT is waiting SIM PUK2 to be given */
  {"PH-NET PIN",    EnterPinCmdResponse_PhNetWait,    AtCmdResultType_Inf}, /* MT is waiting network personalization password to be given */
  {"PH-NETSUB PIN", EnterPinCmdResponse_PhNetsubWait, AtCmdResultType_Inf}, /* MT is waiting network subset personalization password to be given */
  {"PH-SP PIN",     EnterPinCmdResponse_PhSpWait,     AtCmdResultType_Inf}, /* MT is waiting service provider personalization password to be given */
  {"PH-CORP PIN",   EnterPinCmdResponse_PhCorpWait,   AtCmdResultType_Inf}, /* MT is waiting corporate personalization password to be given */
  {"PH-SIM PIN",    EnterPinCmdResponse_PhSimWait,    AtCmdResultType_Inf}, /* MT is waiting phone to SIM/UICC card password to be given */
#endif
  {"READY",         EnterPinCmdResponse_NoSim,        AtCmdResultType_Inf}, /* MT is not pending for any password */
};

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

uint32_t send_command(SARA_U270_Control_t *modem, const char* cmdfmt, ...);

int32_t receive_response(SARA_U270_Control_t *modem,
    CmdRespHandler_t handler,
    void* param,
    uint32_t timeout);

int _findSocket(SARA_U270_Control_t* modem, int handle);
uint32_t _socketFree(SARA_U270_Control_t* modem, int socket);

int32_t mt_power_on(SARA_U270_Control_t* modem);
uint32_t mt_init(SARA_U270_Control_t* modem);
int32_t mt_echo_set(SARA_U270_Control_t* modem, EchoStatus_e echo);
int32_t mt_error_format_set(SARA_U270_Control_t* modem, ErrorReportFormat_e err);
int32_t mt_event_reporting_set(SARA_U270_Control_t* modem, uint32_t mode,
    uint32_t keyp, uint32_t disp, uint32_t ind, uint32_t bfr);
int32_t mt_baudrate_set(SARA_U270_Control_t* modem, uint32_t baudrate);
Sim mt_pin_status_get(SARA_U270_Control_t* modem);
Sim mt_enter_pin(SARA_U270_Control_t* modem, const char* pin);

/* response callbacks */
int _cbCPIN(int type, const uint8_t* buf, int len, Sim* sim);
int _cbCCID(int type, const char* buf, int len, char* ccid);
int _cbString(int type, const char* buf, int len, char* str);


/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

ModemHandle_t SARA_U270_Open(ModemConfiguration_t *mdmconfig)
{
  ssUartConfigType config;
  SARA_U270_Control_t *modem = NULL;
  int32_t huart;

  config.baudrate = MODEM_UART_BAUDRATE;
  config.FlowControl = MODEM_UART_FLOWCONTROL;
  config.Mode = MODEM_UART_MODE;
  config.Parity = MODEM_UART_PARITY;
  config.StopBits = MODEM_UART_STOPBITS;
  config.WordLength = MODEM_UART_WORDLENGTH;

  huart = ssUartOpen(MODEM_UART, &config, MODEM_UART_BUFFER_SIZE);
  if(huart < 0)
  {
    goto SARA_U270_Open_Error_0;
  }

  modem = (SARA_U270_Control_t *)pvPortMalloc(sizeof(SARA_U270_Control_t));
  if(modem == NULL)
  {
    goto SARA_U270_Open_Error_1;
  }

  modem->huart = huart;

  modem->rx_buf = (uint8_t *)pvPortMalloc(MODEM_RX_BUF_SIZE);
  if(modem->rx_buf == NULL)
  {
    goto SARA_U270_Open_Error_2;
  }
  modem->tx_buf = (uint8_t *)pvPortMalloc(MODEM_TX_BUF_SIZE);
  if(modem->tx_buf == NULL)
  {
    goto SARA_U270_Open_Error_3;
  }
  /* Echo is on by default */
  modem->echo = Echo_On;

  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "[ Modem::powerOn ] = = = = = = = = = = = = = =");

  if(mt_power_on(modem) != RESP_OK)
  {
    goto SARA_U270_Open_Error_4;
  }

  if(mt_echo_set(modem, Echo_Off) != RESP_OK)
  {
    goto SARA_U270_Open_Error_4;
  }

  if(mt_error_format_set(modem, ErrorReportFormat_Verbose) !=  RESP_OK)
  {
    goto SARA_U270_Open_Error_4;
  }

  if(mt_event_reporting_set(modem, 1, 0, 0, 2, 1) !=  RESP_OK)
  {
    goto SARA_U270_Open_Error_4;
  }

  if(mt_baudrate_set(modem, 115200) !=  RESP_OK)
  {
    goto SARA_U270_Open_Error_4;
  }
  // wait some time until baudrate is applied
  osDelay(100);

  //mt_pin_status_get(modem);

  for (int i = 0; (i < 5) && (modem->dev.sim != SIM_READY) && !modem->cancel_all_operations; i++)
  {
    modem->dev.sim = mt_pin_status_get(modem);

#if 0
    // having an error here is ok (sim may still be initializing)
    if ((RESP_OK != ret) && (RESP_ERROR != ret))
    {
      goto failure;
    }
    else if (i==4 && (RESP_OK != ret) && !retried_after_reset) {
        retried_after_reset = true; // only perform reset & retry sequence once
        i = 0;
        if(!powerOff())
            reset();
        /* Power on the modem and perform basic initialization again */
        if (!_powerOn())
            goto failure;
    }
#endif

    // Enter PIN if needed
    if (modem->dev.sim == SIM_PIN)
    {
      if (!mdmconfig->simpin)
      {
        ssLoggingPrint(ESsLoggingLevel_Error, 0, "SIM PIN not available\r\n");
        goto SARA_U270_Open_Error_4;
      }
      modem->dev.sim = mt_enter_pin(modem, mdmconfig->simpin);
      if (modem->dev.sim != SIM_READY)
      {
        ssLoggingPrint(ESsLoggingLevel_Error, 0, "SIM PIN invalid\r\n");
        goto SARA_U270_Open_Error_4;
      }
    }
  }
  if (modem->dev.sim != SIM_READY)
  {
    if (modem->dev.sim == SIM_MISSING)
    {
      ssLoggingPrint(ESsLoggingLevel_Error, 0, "SIM not inserted\r\n");
    }
    goto SARA_U270_Open_Error_4;
  }

  return (ModemHandle_t) modem;

SARA_U270_Open_Error_4:
  vPortFree(modem->tx_buf);
SARA_U270_Open_Error_3:
  vPortFree(modem->rx_buf);
SARA_U270_Open_Error_2:
  vPortFree(modem);
SARA_U270_Open_Error_1:
  /* @TODO: close uart */
SARA_U270_Open_Error_0:
  return NULL;
}


uint32_t SARA_U270_connect(ModemHandle_t modemHandle)
{
  SARA_U270_Control_t *modem = (SARA_U270_Control_t *)modemHandle;

  mt_init(modem);
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

uint32_t send_command(SARA_U270_Control_t *modem, const char* cmdfmt, ...)
{
  va_list args;
  int len;

  va_start(args, cmdfmt);
  len = vsnprintf((char *)modem->tx_buf, MODEM_TX_BUF_SIZE-1, cmdfmt, args);
  va_end(args);

  /* add command termination char */
  modem->tx_buf[len++] = S3_CHAR;
  modem->tx_buf[len] = '\0';

  ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, (const char *)modem->tx_buf, "[AT send]     ");

  return ssUartWrite(modem->huart, modem->tx_buf, len);
}


#if 0
#define RX_STATE_IDLE   0
#define RX_STATE_ECHO   1
#define RX_STATE_RESP   2
#define RX_STATE_SOL    3
#define RX_STATE_TEXT   4
#define RX_STATE_EOL    5
#define RX_STATE_PARSE  6
#define RX_STATE_END    7
#define RX_STATE_ERROR  8

uint32_t receive_response(SARA_U270_Control_t *modem,
    CmdRespHandler_t handler,
    void* param,
    uint32_t timeout)
{
  uint32_t status = 0;
  uint32_t nbyte = 0;
  uint8_t c;
  uint32_t state = RX_STATE_IDLE;
  uint8_t *response_str = NULL;
  uint8_t *current_line = NULL;
  uint32_t response_len = 0;
  uint32_t current_line_len = 0;

  if (modem->echo)
  {
    state = RX_STATE_ECHO;
  }
  else
  {
    state = RX_STATE_RESP;
  }

  do
  {
    c = ssUartGetc(modem->huart, timeout);
    modem->rx_buf[nbyte] = c;

    switch (state)
    {
    case RX_STATE_IDLE:
      /* nothing really to do */
      break;
    case RX_STATE_ECHO:
      if (c == S3_CHAR)
      {
        /* echo is terminated with S3 character ('\r' by default) */
        state = RX_STATE_RESP;
      }
      break;
    case RX_STATE_RESP:
      /* Wait for the start of response line.
       * Response line is started by S3 character ('\r' by default).
       * It is an error if anything else is received */
      if (c == S3_CHAR)
      {
        state = RX_STATE_SOL;
      }
      else
      {
        state = RX_STATE_ERROR;
      }
      break;
    case RX_STATE_SOL:
      /* S3 should be followed by the S4 character ('\n' by default). */
      if (c == S4_CHAR)
      {
        state = RX_STATE_TEXT;
      }
      else
      {
        state = RX_STATE_ERROR;
      }
      break;
    case RX_STATE_TEXT:
      /* On enter state:
       * Save the pointer to the start of the response text, it will be parsed in handler callback.
       * Save the pointer to the start of the current, it will be parsed after complete line is received. */
      if (response_str == NULL)
      {
        response_str = &(modem->rx_buf[nbyte]);
      }
      if (current_line == NULL)
      {
        current_line = &(modem->rx_buf[nbyte]);
      }

      /* Response line is started by S3 character ('\r' by default). */
      if (c == S3_CHAR)
      {
        state = RX_STATE_EOL;
      }
      else
      {
        current_line_len++;
        response_len++;
      }

      break;
    case RX_STATE_EOL:
      /* S3 should be followed by the S4 character ('\n' by default). */
      if (c == S4_CHAR)
      {
        int i = 0, found = 0;
        /* End of the line, check if it is the final result */
        for (i = 0; (i < sizeof(AtCmdResultCodeTable) / sizeof(AtCmdResultCodeEntry_t)) && (found == 0); i++)
        {
          if (AtCmdResultCodeTable[i].type == AtCmdResultType_Final)
          {
            if ((current_line_len == strlen(AtCmdResultCodeTable[i].verbose))
                && (memcmp(current_line, AtCmdResultCodeTable[i].verbose,
                    current_line_len) == 0))
            {
              /* final response */
              status = AtCmdResultCodeTable[i].code;
              found = 1;
            }
          }
        }
        if(found)
        {
          state = RX_STATE_END;
          if(current_line == response_str)
          {
            /* Response string contained only the final result, don't send it to the handler */
            response_str = NULL;
            response_len = 0;
          }
        }
        else
        {
          state = RX_STATE_RESP;
          current_line_len = 0;
          current_line = NULL;
        }
      }
      else
      {
        state = RX_STATE_ERROR;
      }
      break;
    case RX_STATE_END:
      break;
    case RX_STATE_ERROR:
      break;
    }
    nbyte++;
  } while ((state != RX_STATE_END) && (state != RX_STATE_ERROR));

  if((state == RX_STATE_END) && (handler != NULL))
  {
    status = handler(status, response_str, response_len, param);
  }

  ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, (const char *)modem->rx_buf, "[AT recv] ");

  return status;
}
#else




#define RESPONSE_STATE_START    0
#define RESPONSE_STATE_HEADER   1
#define RESPONSE_STATE_TEXT     2
#define RESPONSE_STATE_TRAILER  3
#define RESPONSE_STATE_END      4

#define RESPONSE_EVENT_TIMEOUT  0
#define RESPONSE_EVENT_CHAR     1

uint32_t _getLine(SARA_U270_Control_t *modem, uint32_t timeout)
{
  uint8_t* line_ptr;
  uint32_t state = RESPONSE_STATE_START;
  uint32_t size = 0;
  uint32_t nbytes = 0;

  line_ptr = modem->rx_buf;

  do
  {
    size = ssUartRead(modem->huart, line_ptr, 1, timeout);
    nbytes += size;

    if(size != 0)
    {
      switch(state)
      {
      case RESPONSE_STATE_START:
        if(*line_ptr == S3_CHAR)
        {
          state = RESPONSE_STATE_HEADER;
        }
        else
        {
          state = RESPONSE_STATE_TEXT;
        }
        break;
      case RESPONSE_STATE_HEADER:
        if(*line_ptr == S4_CHAR)
        {
          state = RESPONSE_STATE_TEXT;
        }
        else
        {
          /* error, start from beginning */
          state = RESPONSE_STATE_START;
        }
        break;
      case RESPONSE_STATE_TEXT:
        if(*line_ptr == S3_CHAR)
        {
          state = RESPONSE_STATE_TRAILER;
        }
        else
        {
          /* error, start from beginning */
          state = RESPONSE_STATE_START;
        }
        break;
      case RESPONSE_STATE_TRAILER:
        if(*line_ptr == S4_CHAR)
        {
          state = RESPONSE_STATE_END;
        }
        else
        {
          /* error, start from beginning */
          state = RESPONSE_STATE_START;
        }
        break;
      case RESPONSE_STATE_END:
        /* should not end here, it is a pseudo state to indicate we are done with reception */
        break;
      }
      line_ptr++;
    }
  } while ((state != RESPONSE_STATE_END) && (size != 0));

  *line_ptr = '\0';

  return nbytes;
}

int _parseFormated(uint8_t *line, int len, const char* fmt)
{
  int o = 0;
  int num = 0;
  if (fmt)
  {
    while (*fmt)
    {
      if (++o > len)
        return WAIT;
      char ch = *line++;
      if (*fmt == '%')
      {
        fmt++;
        if (*fmt == 'd')
        { // numeric
          fmt++;
          num = 0;
          while (ch >= '0' && ch <= '9')
          {
            num = num * 10 + (ch - '0');
            if (++o > len)
              return WAIT;
            ch = *line++;
          }
        }
        else if (*fmt == 'c')
        { // char buffer (takes last numeric as length)
          fmt++;
          while (num--)
          {
            if (++o > len)
              return WAIT;
            ch = *line++;
          }
        }
        else if (*fmt == 's')
        {
          fmt++;
          if (ch != '\"')
            return NOT_FOUND;
          do
          {
            if (++o > len)
              return WAIT;
            ch = *line++;
          } while (ch != '\"');
          if (++o > len)
            return WAIT;
          ch = *line++;
        }
      }
      if (*fmt++ != ch)
        return NOT_FOUND;
    }
  }
  return o;
}

int _parseMatch(uint8_t *line, int len, const char* sta, const char* end)
{
  int o = 0;
  if (sta)
  {
    while (*sta)
    {
      if (++o > len)
        return WAIT;
      char ch = *line++;
      if (*sta++ != ch)
        return NOT_FOUND;
    }
  }
  if (!end)
    return o; // no termination
  // at least any char
  if (++o > len)
    return WAIT;
  line++;
  // check the end
  int x = 0;
  while (end[x])
  {
    if (++o > len)
      return WAIT;
    char ch = *line++;
    x = (end[x] == ch) ? x + 1 : (end[0] == ch) ? 1 : 0;
  }
  return o;
}

static struct
{
  const char* fmt;                              int type;
} lutF[] = {
  { "\r\n+USORD: %d,%d,\"%c\"",                   TYPE_PLUS       },
  { "\r\n+USORF: %d,\"" IPSTR "\",%d,%d,\"%c\"",  TYPE_PLUS       },
  { "\r\n+URDFILE: %s,%d,\"%c\"",                 TYPE_PLUS       },
};

static struct
{
  const char* sta;          const char* end;    int type;
} lut[] = {
  { "\r\nOK\r\n",             NULL,               TYPE_OK         },
  { "\r\nERROR\r\n",          NULL,               TYPE_ERROR      },
  { "\r\n+CME ERROR:",        "\r\n",             TYPE_ERROR      },
  { "\r\n+CMS ERROR:",        "\r\n",             TYPE_ERROR      },
  { "\r\nRING\r\n",           NULL,               TYPE_RING       },
  { "\r\nCONNECT\r\n",        NULL,               TYPE_CONNECT    },
  { "\r\nNO CARRIER\r\n",     NULL,               TYPE_NOCARRIER  },
  { "\r\nNO DIALTONE\r\n",    NULL,               TYPE_NODIALTONE },
  { "\r\nBUSY\r\n",           NULL,               TYPE_BUSY       },
  { "\r\nNO ANSWER\r\n",      NULL,               TYPE_NOANSWER   },
  { "\r\n+",                  "\r\n",             TYPE_PLUS       },
  { "\r\n@",                  NULL,               TYPE_PROMPT     }, // Sockets
  { "\r\n>",                  NULL,               TYPE_PROMPT     }, // SMS
  { "\n>",                    NULL,               TYPE_PROMPT     }, // File
  { "\r\nABORTED\r\n",        NULL,               TYPE_ABORTED    }, // Current command aborted
  { "\r\n\r\n",               "\r\n",             TYPE_DBLNEWLINE }, // Double CRLF detected
  { "\r\n",                   "\r\n",             TYPE_UNKNOWN    }, // If all else fails, break up generic strings
};


int32_t parse_line(uint8_t *buf, uint32_t len)
{
  int unkn = 0;
  uint8_t *bufptr = buf;
  uint32_t size = len;

  while (size > 0)
  {

    for (int i = 0; i < (int) (sizeof(lutF) / sizeof(*lutF)); i++)
    {
      bufptr = buf + unkn;

      int ln = _parseFormated(bufptr, size, lutF[i].fmt);
      if (ln == WAIT && ((len - size) > 0))
      {
        return WAIT;
      }
      if ((ln != NOT_FOUND) && (unkn > 0))
      {
        return TYPE_UNKNOWN | unkn;
      }
      if (ln > 0)
      {
        return lutF[i].type | ln;
      }
    }

    for (int i = 0; i < (int) (sizeof(lut) / sizeof(*lut)); i++)
    {
      bufptr = buf + unkn;
      int ln = _parseMatch(bufptr, size, lut[i].sta, lut[i].end);
      if (ln == WAIT && ((len - size) > 0))
      {
        return WAIT;
      }
      // Double CRLF detected, discard it.
      // This resolves a case on G350 where "\r\n" is generated after +USORF response, but missing
      // on U260/U270, which would otherwise generate "\r\n\r\nOK\r\n" which is not parseable.
      if ((ln > 0) && (lut[i].type == TYPE_DBLNEWLINE) && (unkn == 0))
      {
        return TYPE_UNKNOWN | 2;
      }
      if ((ln != NOT_FOUND) && (unkn > 0))
      {
        return TYPE_UNKNOWN | unkn;
      }
      if (ln > 0)
      {
        return lut[i].type | ln;
      }
    }
    // UNKNOWN
    unkn++;
    len--;
  }
  return WAIT;
}



int32_t receive_response(SARA_U270_Control_t *modem,
    CmdRespHandler_t handler,
    void* param,
    uint32_t timeout)
{
  uint32_t nbytes = 0;
  uint32_t offset = 0;
  uint8_t *resp;

  do
  {
    nbytes = _getLine(modem, timeout);
    resp = modem->rx_buf;
    offset = 0;

    if(nbytes>0)
    {
      while(offset < nbytes)
      {
        resp += offset;
        int ret = parse_line(resp, nbytes);

        if ((ret != WAIT) && (ret != NOT_FOUND))
        {
          int type = TYPE(ret);
          offset += LENGTH(ret);

          const char* s = (type == TYPE_UNKNOWN)? "UNK" :
                          (type == TYPE_TEXT)   ? "TXT"  :
                          (type == TYPE_OK   )  ? "OK "  :
                          (type == TYPE_ERROR)  ? "ERR"  :
                          (type == TYPE_ABORTED) ? "ABT" :
                          (type == TYPE_PLUS)   ? " + " :
                          (type == TYPE_PROMPT) ? " > " :  "..." ;
          ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, (const char *)resp, "[AT recv] %s ", s);

          // handle unsolicited commands here
          if (type == TYPE_PLUS) {
            const uint8_t* cmd = resp+3;
            int a, b, c, d, r;
            char s[32];

            // SMS Command ---------------------------------
            // +CNMI: <mem>,<index>
            if (sscanf(cmd, "CMTI: \"%*[^\"]\",%d", &a) == 1)
            {
              ssLoggingPrint(ESsLoggingLevel_Info, 0, "New SMS at index %d\r\n", a);
              //?if (sms_cb) SMSreceived(a);
            }
            else if ((sscanf(cmd, "CIEV: 9,%d", &a) == 1))
            {
              ssLoggingPrint(ESsLoggingLevel_Info, 0, "CIEV matched: 9,%d\r\n", a);
              // Wait until the system is attached before attempting to act on GPRS detach
              if (modem->attached)
              {
                modem->attached_urc = (a==2)?1:0;
                if (!modem->attached_urc)
                {
                  //?ARM_GPRS_TIMEOUT(15*1000); // If detached, set WDT
                }
                else
                {
                  //?CLR_GPRS_TIMEOUT(); // else if re-attached clear WDT.
                }
              }
            // Socket Specific Command ---------------------------------
            } // +UUSORD: <socket>,<length>
            else if ((sscanf(cmd, "UUSORD: %d,%d", &a, &b) == 2))
            {
              int socket = _findSocket(modem, a);
              ssLoggingPrint(ESsLoggingLevel_Info, 0, "Socket %d: handle %d has %d bytes pending\r\n", socket, a, b);
              if (socket != MDM_SOCKET_ERROR)
                modem->sockets[socket].pending = b;
            } // +UUSORF: <socket>,<length>
            else if ((sscanf(cmd, "UUSORF: %d,%d", &a, &b) == 2))
            {
              int socket = _findSocket(modem, a);
              ssLoggingPrint(ESsLoggingLevel_Info, 0, "Socket %d: handle %d has %d bytes pending\r\n", socket, a, b);
              if (socket != MDM_SOCKET_ERROR)
                modem->sockets[socket].pending = b;
            } // +UUSOCL: <socket>
            else if ((sscanf(cmd, "UUSOCL: %d", &a) == 1))
            {
              int socket = _findSocket(modem, a);
              ssLoggingPrint(ESsLoggingLevel_Info, 0, "Socket %d: handle %d closed by remote host\r\n", socket, a);
              if (socket != MDM_SOCKET_ERROR)
              {
                _socketFree(modem, socket);
              }
            }

            // GSM/UMTS Specific -------------------------------------------
            // +UUPSDD: <profile_id>
            if (sscanf(cmd, "UUPSDD: %s", s) == 1)
            {
              ssLoggingPrint(ESsLoggingLevel_Info, 0, "UUPSDD: %s matched\r\n", PROFILE);
              if ( !strcmp(s, PROFILE) )
              {
                modem->ip = NOIP;
                modem->attached = 0;
                ssLoggingPrint(ESsLoggingLevel_Info, 0, "PDP context deactivated remotely!\r\n");
                // PDP context was remotely deactivated via URC,
                // Notify system of disconnect.
                //?HAL_NET_notify_dhcp(0);
              }
            }
            else
            {
              // +CREG|CGREG: <n>,<stat>[,<lac>,<ci>[,AcT[,<rac>]]] // reply to AT+CREG|AT+CGREG
              // +CREG|CGREG: <stat>[,<lac>,<ci>[,AcT[,<rac>]]]     // URC
              b = (int)0xFFFF; c = (int)0xFFFFFFFF; d = -1;
              r = sscanf(cmd, "%s %*d,%d,\"%x\",\"%x\",%d",s,&a,&b,&c,&d);
              if (r <= 1)
                r = sscanf(cmd, "%s %d,\"%x\",\"%x\",%d",s,&a,&b,&c,&d);
              if (r >= 2)
              {
                Reg *reg = !strcmp(s, "CREG:")  ? &modem->net.csd :
                           !strcmp(s, "CGREG:") ? &modem->net.psd : NULL;
                if (reg)
                {
                  // network status
                  if      (a == 0) *reg = REG_NONE;     // 0: not registered, home network
                  else if (a == 1) *reg = REG_HOME;     // 1: registered, home network
                  else if (a == 2) *reg = REG_NONE;     // 2: not registered, but MT is currently searching a new operator to register to
                  else if (a == 3) *reg = REG_DENIED;   // 3: registration denied
                  else if (a == 4) *reg = REG_UNKNOWN;  // 4: unknown
                  else if (a == 5) *reg = REG_ROAMING;  // 5: registered, roaming
                  if ((r >= 3) && (b != (int)0xFFFF))      modem->net.lac = b; // location area code
                  if ((r >= 4) && (c != (int)0xFFFFFFFF))  modem->net.ci  = c; // cell ID
                  // access technology
                  if (r >= 5)
                  {
                    if      (d == 0) modem->net.act = ACT_GSM;      // 0: GSM
                    else if (d == 1) modem->net.act = ACT_GSM;      // 1: GSM COMPACT
                    else if (d == 2) modem->net.act = ACT_UTRAN;    // 2: UTRAN
                    else if (d == 3) modem->net.act = ACT_EDGE;     // 3: GSM with EDGE availability
                    else if (d == 4) modem->net.act = ACT_UTRAN;    // 4: UTRAN with HSDPA availability
                    else if (d == 5) modem->net.act = ACT_UTRAN;    // 5: UTRAN with HSUPA availability
                    else if (d == 6) modem->net.act = ACT_UTRAN;    // 6: UTRAN with HSDPA and HSUPA availability
                  }
                }
              }
            }
          } // end ==TYPE_PLUS
          if (handler)
          {
            int len = LENGTH(ret);
            int ret = handler(type, resp, len, param);
            if (WAIT != ret)
              return ret;
          }
          if (type == TYPE_OK)
            return RESP_OK;
          if (type == TYPE_ERROR)
            return RESP_ERROR;
          if (type == TYPE_PROMPT)
            return RESP_PROMPT;
          if (type == TYPE_ABORTED)
            return RESP_ABORTED; // This means the current command was ABORTED, so retry your command if critical.
        }
      }
    }
  } while(nbytes>0);

  return WAIT;
}

#endif

int32_t mt_power_on(SARA_U270_Control_t* modem)
{
  int32_t status = WAIT;

  for (int i = 0; (i < MODEM_POWER_ON_RETRY_MAX) && (status != RESP_OK); i++)
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

    // check interface
    send_command(modem, "AT");
    status = receive_response(modem, NULL, NULL, 1000);
  }

  return status;
}

uint32_t mt_init(SARA_U270_Control_t* modem)
{
  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "[ Modem::init ] = = = = = = = = = = = = = =");

  // Returns the product serial number, IMEI (International Mobile Equipment Identity)
  send_command(modem, "AT+CGSN");
  if (RESP_OK != receive_response(modem, _cbString, modem->dev.imei, 10000))
    return ModemResult_Error;

  // get the manufacturer
  send_command(modem, "AT+CGMI");
  if (RESP_OK != receive_response(modem, _cbString, modem->dev.manu, 10000))
    return ModemResult_Error;

  // get the model identification
  send_command(modem, "AT+CGMM");
  if (RESP_OK != receive_response(modem, _cbString, modem->dev.model, 10000))
    return ModemResult_Error;

  // get the version
  send_command(modem, "AT+CGMR");
  if (RESP_OK != receive_response(modem, _cbString, modem->dev.ver, 10000))
    return ModemResult_Error;
  // Returns the ICCID (Integrated Circuit Card ID) of the SIM-card.
  // ICCID is a serial number identifying the SIM.
  send_command(modem, "AT+CCID");
  if (RESP_OK != receive_response(modem, _cbCCID, modem->dev.ccid, 10000))
    return ModemResult_Error;

  // enable power saving
  if (modem->dev.lpm != LPM_DISABLED)
  {
    // enable power saving (requires flow control, cts at least)
    send_command(modem, "AT+UPSV=1");
    if (RESP_OK != receive_response(modem, NULL, NULL, 10000))
      return ModemResult_Error;
    modem->dev.lpm = LPM_ACTIVE;
  }
  // Setup SMS in text mode
  send_command(modem, "AT+CMGF=1");
  if (RESP_OK != receive_response(modem, NULL, NULL, 10000))
    return ModemResult_Error;
  // setup new message indication
  send_command(modem, "AT+CNMI=2,1");
  if (RESP_OK != receive_response(modem, NULL, NULL, 10000))
    return ModemResult_Error;
  // Request IMSI (International Mobile Subscriber Identification)
  send_command(modem, "AT+CIMI");
  if (RESP_OK != receive_response(modem, _cbString, modem->dev.imsi, 10000))
    return ModemResult_Error;

  return ModemResult_OK;
}


int32_t mt_echo_set(SARA_U270_Control_t* modem, EchoStatus_e echo)
{
  int32_t status;

  send_command(modem, "ATE%d", echo);
  status = receive_response(modem, NULL, NULL, 1000);

  if(status == RESP_OK)
  {
    modem->echo = echo;
  }

  return status;
}

int32_t mt_error_format_set(SARA_U270_Control_t* modem, ErrorReportFormat_e err)
{
  int32_t status;

  send_command(modem, "AT+CMEE=%d", err);
  status = receive_response(modem, NULL, NULL, 1000);

  return status;
}


int32_t mt_event_reporting_set(SARA_U270_Control_t* modem, uint32_t mode,
    uint32_t keyp, uint32_t disp, uint32_t ind, uint32_t bfr)
{
  int32_t status;

  send_command(modem, "AT+CMER=%d,%d,%d,%d,%d", mode, keyp, disp, ind, bfr);
  status = receive_response(modem, NULL, NULL, 1000);

  return status;
}

int32_t mt_baudrate_set(SARA_U270_Control_t* modem, uint32_t baudrate)
{
  int32_t status;

  send_command(modem, "AT+IPR=%d", baudrate);
  status = receive_response(modem, NULL, NULL, 1000);

  return status;
}

Sim mt_pin_status_get(SARA_U270_Control_t* modem)
{
  Sim status = SIM_UNKNOWN;

  send_command(modem, "AT+CPIN?");
  if(receive_response(modem, _cbCPIN, &status, MODEM_DEFAULT_TIMEOUT) != RESP_OK)
  {
    status = SIM_UNKNOWN;
  }

  return status;
}

Sim mt_enter_pin(SARA_U270_Control_t* modem, const char* pin)
{
  Sim status = SIM_UNKNOWN;

  send_command(modem, "AT+CPIN=%s", pin);
  if(receive_response(modem, _cbCPIN, &status, MODEM_DEFAULT_TIMEOUT) != RESP_OK)
  {
    status = SIM_UNKNOWN;
  }

  return status;
}

int _findSocket(SARA_U270_Control_t* modem, int handle)
{
  for (int socket = 0; socket < NUMSOCKETS; socket ++)
  {
    if (modem->sockets[socket].handle == handle)
      return socket;
  }
  return MDM_SOCKET_ERROR;
}

uint32_t _socketFree(SARA_U270_Control_t* modem, int socket)
{
  uint32_t ok = 0;
  //?LOCK();
  if ((socket >= 0) && (socket < NUMSOCKETS))
  {
      if (modem->sockets[socket].handle != MDM_SOCKET_ERROR)
      {
        ssLoggingPrint(ESsLoggingLevel_Info, 0, "socketFree(%d)\r\n",  socket);
        modem->sockets[socket].handle     = MDM_SOCKET_ERROR;
        modem->sockets[socket].timeout_ms = TIMEOUT_BLOCKING;
        modem->sockets[socket].connected  = 0;
        modem->sockets[socket].pending    = 0;
        modem->sockets[socket].open       = 0;
      }
      ok = 1;
  }
  //?UNLOCK();
  return ok; // only false if invalid socket
}


/* response callbacks */

int _cbString(int type, const char* buf, int len, char* str)
{
    if (str && (type == TYPE_UNKNOWN)) {
        if (sscanf(buf, "\r\n%s\r\n", str) == 1)
            /*nothing*/;
    }
    return WAIT;
}

int _cbCPIN(int type, const uint8_t* buf, int len, Sim* sim)
{
  if (sim)
  {
    if (type == TYPE_PLUS)
    {
      char s[16];
      if (sscanf(buf, "\r\n+CPIN: %[^\r]\r\n", s) >= 1)
      {
        *sim = (0 == strcmp("READY", s)) ? SIM_READY : SIM_PIN;
      }
    }
    else if (type == TYPE_ERROR)
    {
      if (strstr(buf, "+CME ERROR: SIM not inserted"))
      {
        *sim = SIM_MISSING;
      }
    }
  }
  return WAIT;
}


int _cbCCID(int type, const char* buf, int len, char* ccid)
{
    if ((type == TYPE_PLUS) && ccid) {
        if (sscanf(buf, "\r\n+CCID: %[^\r]\r\n", ccid) == 1) {
            //INFO("Got CCID: %s\r\n", ccid);
        }
    }
    return WAIT;
}

#ifdef __cplusplus
}
#endif

