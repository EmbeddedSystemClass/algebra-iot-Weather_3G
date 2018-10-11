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

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"

#include "cmsis_os.h"

#include "hal_modem.h"

#include "ssUart.h"
#include "ssLogging.h"
#include "ssSocket.h"  
#include "ATCmdParser.h"
#include "ssDevMan.h"
#include "ssModem.h"


/*------------------------- MACRO DEFINITIONS --------------------------------*/

#define PROFILE "0"

#define AT_PARSER_TIMEOUT       5000 // Milliseconds

#define UNNATURAL_STRING "\x01"

#define u_stringify(a) str(a)
#define str(a) #a

/** Socket timeout value in milliseconds.
* Note: the sockets layer above will retry the
* call to the functions here when they return NSAPI_ERROR_WOULD_BLOCK
* and the user has set a larger timeout or full blocking.
*/
#define SOCKET_TIMEOUT 1000

/** The maximum number of bytes in a packet that can be written
* to the AT interface in one go.
*/
#define MAX_WRITE_SIZE 1024

/** The maximum number of bytes in a packet that can be read from
* from the AT interface in one go.
*/
#define MAX_READ_SIZE 1024

#define MODEM_TIMEOUT_DEFAULT 1000
#define RECV_TASK_TIMEOUT     1/portTICK_PERIOD_MS  // Merkat experimenting with value

/*------------------------- TYPE DEFINITIONS ---------------------------------*/


/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PRIVATE VARIABLES --------------------------------*/
SemaphoreHandle_t mtx;
const char *ran_type_name_table[] =
{
  "GSM",
  "GSM",
  "UTRAN",
  "EDGE",
  "HSDPA",
  "HSUPA",
  "HSDPA_HSUPA",
  "LTE",
  "EC_GSM_IoT",
  "E_UTRAN_NB_S1"
};

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/
bool power_up(modem_t *self);
bool reset(modem_t *self);

bool init_sim_card(modem_t *self);
bool set_device_identity(modem_t *self);
bool device_init(modem_t *self);

bool activate_profile(modem_t *self, const char* apn, const char* username, const char* password);

bool get_iccid(modem_t *self);
bool get_imsi(modem_t *self);
bool get_imei(modem_t *self);
bool get_meid(modem_t *self);

bool is_registered_csd(modem_t *self);
bool is_registered_psd(modem_t *self);
bool is_registered_eps(modem_t *self);

void set_rat(modem_t *self, int AcTStatus);
void set_nwk_reg_status_csd(modem_t *self, int status);

int read_at_to_char(modem_t *self, char * buf, int size, char end);
void parser_abort_cb(void *param);

void CMX_ERROR_URC(void *param);
void CREG_URC(void *param);
void UUSORD_URC(void *param);
void UUSORF_URC(void *param);
void UUSOCL_URC(void *param);


/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

static void LOCK()
{
  xSemaphoreTake(mtx, portMAX_DELAY);
}

static void UNLOCK()
{
  xSemaphoreGive(mtx);
}

modem_t *modem_create(void)
{
  modem_t *modem;
  
  modem = pvPortMalloc(sizeof(modem_t));
  assert(modem);
  
  /* open modem uart */
  ssUartConfigType config;
  
  hal_modem_init();
  
  config.baudrate = 115200;
  config.FlowControl = UART_HWCONTROL_RTS_CTS;
  config.Mode = UART_MODE_TX_RX;
  config.Parity = UART_PARITY_NONE;
  config.StopBits = UART_STOPBITS_1;
  config.WordLength = UART_WORDLENGTH_8B;
  modem->fd = ssUartOpen(USART3, &config, 1024);
  configASSERT(modem->fd >= 0);
  
  modem->at = atparser_create(modem->fd);
  assert(modem->at);
  
  modem->at_timeout = MODEM_TIMEOUT_DEFAULT;
  atparser_set_timeout(modem->at, modem->at_timeout);
  
  modem->sim_pin_enabled = false;
  modem->pin = NULL;
  
  modem->modem_initialised = false;
  
  modem->apn = NULL;
  modem->uname = NULL;
  modem->pwd = NULL;
  
  for(int i=0; i<SOCKET_COUNT; i++)
  {
    modem->sockets[i].state = SOCKET_CLOSED;
    modem->sockets[i].pending = 0;
    modem->sockets[i].buffer = NULL;
  }
  
  // Error cases, out of band handling
  atparser_oob(modem->at, "ERROR", parser_abort_cb, modem);
  atparser_oob(modem->at, "+CME ERROR", CMX_ERROR_URC, modem);
  atparser_oob(modem->at, "+CMS ERROR", CMX_ERROR_URC, modem);
  
  // Registration status, out of band handling
  atparser_oob(modem->at, "+CREG", CREG_URC, modem);
  
  atparser_oob(modem->at, "+UUSORD", UUSORD_URC, modem);
  atparser_oob(modem->at, "+UUSORF", UUSORF_URC, modem);
  atparser_oob(modem->at, "+UUSOCL", UUSOCL_URC, modem);
  
  mtx = xSemaphoreCreateMutex();
  
  return modem;
}

void modem_destroy(modem_t *self)
{
  atparser_destroy(self->at);
  //ssUartClose(self->fd);
  vPortFree(self);
}

bool modem_init(modem_t *self, const char *pin)
{
  /* power up */
  self->pin = pin;
  if(!self->modem_initialised)
  {
    assert(reset(self));
    assert(power_up(self));
    assert(init_sim_card(self));
    assert(set_device_identity(self));
    assert(device_init(self));
    assert(device_init(self));
    assert(get_imsi(self));
    assert(get_imei(self));
    assert(get_meid(self));
    self->modem_initialised = true;
  }
  
  return self->modem_initialised;
}

bool modem_nwk_register(modem_t *self)
{
  bool atSuccess = false;
  bool registered = false;
  int status;
  LOCK();
  
  if (!is_registered_psd(self) && !is_registered_csd(self) && !is_registered_eps(self))
  {
    ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Searching Network...");
    // Enable the packet switched and network registration unsolicited result codes
    if (atparser_send(self->at, "AT+CREG=1") && atparser_recv(self->at, "OK") &&
        atparser_send(self->at, "AT+CGREG=1") && atparser_recv(self->at, "OK"))
    {
      atSuccess = true;
      if (atparser_send(self->at, "AT+CEREG=1"))
      {
        atparser_recv(self->at, "OK");
        // Don't check return value as this works for LTE only
      }
      
      if (atSuccess)
      {
        // See if we are already in automatic mode
        if (atparser_send(self->at, "AT+COPS?") && atparser_recv(self->at, "+COPS: %d", &status) &&
            atparser_recv(self->at, "OK"))
        {
          // If not, set it
          if (status != 0)
          {
            /* Don't check return code here as there's not much
               we can do if this fails. */
        	// @TODO> add error handling
            if(atparser_send(self->at, "AT+COPS=0"))
            {
              atparser_recv(self->at, "OK");
            }
          }
        }
        
        // Query the registration status directly as well,
        // just in case
        if (atparser_send(self->at, "AT+CREG?") && atparser_recv(self->at, "OK")) {
          // Answer will be processed by URC
        }
        if (atparser_send(self->at, "AT+CGREG?") && atparser_recv(self->at, "OK")) {
          // Answer will be processed by URC
        }
        if (atparser_send(self->at, "AT+CEREG?")) {
          atparser_recv(self->at, "OK");
          // Don't check return value as this works for LTE only
        }
      }
    }
    // Wait for registration to succeed
    atparser_set_timeout(self->at, 1000);
    for (int waitSeconds = 0; !registered && (waitSeconds < 180); waitSeconds++)
    {
      registered = is_registered_psd(self) || is_registered_csd(self) || is_registered_eps(self);
      atparser_recv(self->at, UNNATURAL_STRING);
    }
    atparser_set_timeout(self->at, self->at_timeout);
    
    if (registered)
    {
      // This should return quickly but sometimes the status field is not returned
      // so make the timeout short
      atparser_set_timeout(self->at, 1000);
      if (atparser_send(self->at, "AT+COPS?") && atparser_recv(self->at, "+COPS: %*d,%*d,\"%*[^\"]\",%d\n", &status))
      {
        set_rat(self, status);
      }
      atparser_set_timeout(self->at, self->at_timeout);
    }
  } else
  {
    registered = true;
  }
  
  UNLOCK();
  return registered;
}

// Perform deregistration.
bool modem_nwk_deregister(modem_t *self)
{
  bool success = false;
  LOCK();
  
  if (atparser_send(self->at, "AT+COPS=2") && atparser_recv(self->at, "OK")) {
    self->dev_info.reg_status_csd = CSD_NOT_REGISTERED_NOT_SEARCHING;
    self->dev_info.reg_status_psd = PSD_NOT_REGISTERED_NOT_SEARCHING;
    self->dev_info.reg_status_eps = EPS_NOT_REGISTERED_NOT_SEARCHING;
    success = true;
  }
  
  UNLOCK();
  return success;
}

// Set APN, user name and password.
void  modem_set_credentials(modem_t *self, const char *apn, const char *uname, const char *pwd)
{
  self->apn = apn;
  self->uname = uname;
  self->pwd = pwd;
}


bool modem_nwk_connect(modem_t *self)
{
  bool success = false;
  int active = 0;
  
  LOCK();
  
  // Check the profile
  if (atparser_send(self->at, "AT+UPSND=" PROFILE ",8") && atparser_recv(self->at, "+UPSND: %*d,%*d,%d\n", &active) &&
      atparser_recv(self->at, "OK"))
  {
    if (active == 0)
    {
      // Attempt to connect
      // Set up APN and IP protocol for PDP context
      success = activate_profile(self, self->apn, self->uname, self->pwd);
    }
    else
    {
      // If the profile is already active, we're good
      success = true;
    }
  }
  
  if (!success)
  {
    // ssLoggingPrint(ESsLoggingLevel_Error, 0, "Failed to connect, check your APN/username/password");
  }
  
  UNLOCK();
  return success;
}


// Disconnect the on board IP stack of the modem.
bool modem_nwk_disconnect(modem_t *self)
{
  bool success = false;
  LOCK();
  
  if (atparser_send(self->at, "AT+UPSDA=" PROFILE ",4") && atparser_recv(self->at, "OK"))
  {
    success = true;
  }
  
  UNLOCK();
  return success;
}


int32_t modem_gethostbyname(modem_t *self,
                            const char *host,
                            uint32_t *address)
{
  int32_t status = -1;
  int at_timeout;
  char ipAddress[SOCK_IP_SIZE] = {0};
  
  
  LOCK();
  // This interrogation can sometimes take longer than the usual 8 seconds
  at_timeout = self->at_timeout;
  atparser_set_timeout(self->at, 60000);
  memset (ipAddress, 0, sizeof (ipAddress)); // Ensure terminator
  if (atparser_send(self->at, "AT+UDNSRN=0,\"%s\"", host) &&
      atparser_recv(self->at, "+UDNSRN: \"%" u_stringify(SOCK_IP_SIZE) "[^\"]\"", ipAddress) &&
        atparser_recv(self->at, "OK"))
  {
    struct in_addr addr;
    if(inet_aton(ipAddress, &addr) != 0)
    {
      *address = addr.s_addr;
      status = 0;
    }
  }
  atparser_set_timeout(self->at, at_timeout);
  UNLOCK();
  
  return status;
}

// Create a socket.
int modem_socket_open(modem_t *self, int protocol)
{
  int socket = SOCKET_INVALID;
  
  /* we support only udp and tcp protocols */
  if(protocol != IPPROTO_TCP &&
     protocol != IPPROTO_UDP && protocol != 0)        /* Merkat 0 is for waakama compatibility */
  {
    return -1;
  }
  
  LOCK();
  
  /* Merkat temp fix for waakama compatibility */
  if (protocol == 0)
    protocol = IPPROTO_UDP;
  
  if (atparser_send(self->at, "AT+USOCR=%d", protocol))
  {
    if (atparser_recv(self->at, "+USOCR: %d\n", &socket) && (socket != SOCKET_INVALID) &&
        atparser_recv(self->at, "OK"))
    {
      ssLoggingPrint(ESsLoggingLevel_Info, 0, "Socket %d was created", socket);
      self->sockets[socket].state = SOCKET_OPENED;
      self->sockets[socket].pending = 0;
      self->sockets[socket].buffer = NULL;
      
    }
  }
  
  UNLOCK();
  return socket;
}

// Create a socket.
bool modem_socket_close(modem_t *self, int socket)
{
  bool success = false;
  LOCK();
  
  if (atparser_send(self->at, "AT+USOCL=%d", socket))
  {
    if (atparser_recv(self->at, "OK"))
    {
      ssLoggingPrint(ESsLoggingLevel_Info, 0, "Socket %d was closed", socket);
      success = true;
    }
    else
    {
      ssLoggingPrint(ESsLoggingLevel_Warning, 0, "Failed to close socket %d", socket);
    }
  }
  
  UNLOCK();
  return success;
}
/*
Enable hex mode configuration for for data sending commands
enable hex mode -- option = 1
disable hex mode -- option = 0
*/
bool modem_set_hex_mode(modem_t *self, uint8_t option)
{
  bool success = false;
  LOCK();
  
  if (atparser_send(self->at, "AT+UDCONF=%d", option))
  {
    if (atparser_recv(self->at, "OK"))
    {
      ssLoggingPrint(ESsLoggingLevel_Info, 0, "Hex mode enabled");
      success = true;
    }
    else
    {
      ssLoggingPrint(ESsLoggingLevel_Warning, 0, "Failed to set hex mode");
    }
  }
  
  UNLOCK();
  return success;
}  

// Send to an IP address.
int16_t modem_socket_sendto(modem_t *self,
                            int socket,
                            const void *message,
                            size_t length,
                            const struct SocketAddress_in *dest_addr)
{
  bool success = true;
  const char *buf = (const char *) message;
  size_t blk = MAX_WRITE_SIZE;
  size_t count = length;
  int32_t nbytes = 0;
  
  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "socket_sendto(%d, %s:%d, %p, %d)",
                 socket, dest_addr->sin_addr, dest_addr->sin_port, message, length);
  
  /* @TODO: check if socket is open */
  
  LOCK();
  
  if (length > MAX_WRITE_SIZE) {
    ssLoggingPrint(ESsLoggingLevel_Warning, 0, "WARNING: packet length %d is too big for one UDP packet (max %d), will be fragmented.", length, MAX_WRITE_SIZE);
  }
  
  while ((count > 0) && success) {
    if (count < blk) {
      blk = count;
    }
    
    if (atparser_send(self->at, "AT+USOST=%d,\"%s\",%d,%d", socket,
                      dest_addr->sin_addr, dest_addr->sin_port, blk) &&
        atparser_recv(self->at, "@")) {
          osDelay(50); // Merkat changed from 200 to 100
          int temp;
          temp = atparser_write(self->at, buf, blk+1);
          //for (size_t i = 0; i <= length; i++) Merkat trying above
          //{
          //atparser_putc(self->at, buf[i]);
          //osDelay(100);
          /* code */
          //}
          // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "atparser_write(%d) returned %d", blk, temp);
          if (atparser_recv(self->at, "OK"))
          {
            nbytes += blk;
          }
          else
          {
            success = false;
          }
        } else {
          success = false;
        }
    
    buf += blk;
    count -= blk;
  }
  
  UNLOCK();
  
    //ssLoggingPrint(ESsLoggingLevel_Debug, 0, "socket_sendto: %d \"%*.*s\"", nbytes, nbytes, nbytes, (char *) message);
    ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, message, nbytes, "[SOCK wr] ");
  return (nbytes > 0) ? nbytes : (-1);
}

int16_t modem_socket_send(modem_t *self, int socket, const void *message, size_t length)
{
  bool success = true;
  const char *buf = (const char *) message;
  size_t blk = MAX_WRITE_SIZE;
  size_t count = length;
  int32_t nbytes = 0;
  
  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "socket_send(%d, %p, %d)",
                 socket, buf, length);
  
  /* @TODO: check if socket is open */
  
  LOCK();
  
  if (length > MAX_WRITE_SIZE) 
  {
    ssLoggingPrint(ESsLoggingLevel_Warning, 0, "WARNING: packet length %d is too big for one UDP packet (max %d), will be fragmented.", length, MAX_WRITE_SIZE);
  }
  
  while ((count > 0) && success) 
  {
    if (count < blk) 
    {
      blk = count;
    }
    
    if (atparser_send(self->at, "AT+USOWR=%d,%d", socket, blk) &&
        atparser_recv(self->at, "@")) 
    {
      osDelay(100);
      //int temp;
      //temp = atparser_write(self->at, buf, blk);
      for (size_t i = 0; i <= length; i++) 
      {
        atparser_putc(self->at, buf[i]);
        //osDelay(100);
        /* code */
        // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "atparser_write(%d) returned %d", blk, temp);
        if (atparser_recv(self->at, "OK"))
        {
          nbytes += blk;
        }
        else
        {
          success = false;
        }
      }
    }
    else 
    {
      success = false;
    }
    
    buf += blk;
    count -= blk;
  }
  
  UNLOCK();
  
  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "socket_sendto: %d \"%*.*s\"", nbytes, nbytes, nbytes, (char *) message);
  ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, message, nbytes, "[SOCK wr] ");
  return (nbytes > 0) ? nbytes : (-1);
}

int16_t modem_socket_recv(modem_t *self, int socket, void *buffer, size_t length)
{
  
  bool success = true;
  char *buf = (char *)buffer;
  int32_t read_blk;
  int32_t count = 0;
  unsigned int usord_sz;
  int read_sz;
  int at_timeout;
  TickType_t xTicksToWait = RECV_TASK_TIMEOUT;
  TimeOut_t xTimeOut;
  
  
  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "socket_recv(%d, %p, %d)",
                 socket, buffer, length);
  
  //timer.start();
  LOCK();
  vTaskSetTimeOutState(&xTimeOut);
  
  while (success && (length > 0)) 
  {
    at_timeout = self->at_timeout;
    atparser_set_timeout(self->at, 1000);
    
    
    read_blk = MAX_READ_SIZE;
    if (read_blk > length) {
      read_blk = length;
    }
    if (self->sockets[socket].pending > 0) 
    {
      ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Socket %d: has %d byte(s) pending",
                     socket, self->sockets[socket].pending);
      
      if (atparser_send(self->at, "AT+USORD=%d,%d", socket, read_blk) &&
          atparser_recv(self->at, "+USORD: %*d,%d,\"", &usord_sz)) 
      {
        // Must use what +USORD returns here as it may be less or more than we asked for
        if (usord_sz > self->sockets[socket].pending) 
        {
          self->sockets[socket].pending = 0;
        }
        else 
        {
          self->sockets[socket].pending -= usord_sz;
        }
        // Note: insert no debug between _at->recv() and _at->read(), no time...
        if (usord_sz > length) 
        {
          usord_sz = length;
        }
        while((usord_sz>0) && success)
        {
          read_sz = atparser_read(self->at, buf, usord_sz);
          if (read_sz > 0)
          {
            ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, buf, read_sz, "[SOCK rd] (%d)-> ", usord_sz);
            count += read_sz;
            buf += read_sz;
            length -= read_sz;
            if ((usord_sz < read_blk) || (usord_sz == MAX_READ_SIZE))
            {
              length = 0; // If we've received less than we asked for, or
              // the max size, then a whole UDP packet has arrived and
              // this means DONE.
            }
            usord_sz -= read_sz;
          }
          else
          {
            // read() should not fail
            success = false;
          }
          ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Socket %d: now has only %d byte(s) pending",
                         socket, self->sockets[socket].pending);
        }
        // Wait for the "OK" before continuing
        atparser_recv(self->at, "OK");
      }
      else
      {
        // Should never fail to do _at->send()/_at->recv()
        success = false;
      }
      //_at->debug_on(_debug_trace_on);
    }
    else if (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) != pdFALSE)
    {
      //Wait for URCs
      atparser_recv(self->at, UNNATURAL_STRING);
      ssLoggingPrint(ESsLoggingLevel_Debug, 0, "SOCKET RECV TIMEOUTED");
      break;
    }
    
    atparser_set_timeout(self->at, at_timeout);
  }
  UNLOCK();
  ulTaskNotifyTake( pdTRUE, xTicksToWait );
  //timer.stop();
  
  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "socket_recv: %d \"%*.*s\"", count, count, count, buf - count);
  
  return count;
}


int16_t modem_socket_recvfrom(modem_t *self,
                              int socket,
                              void *buffer,
                              size_t length,
                              struct SocketAddress_in *restrict address)
{
  
  bool success = true;
  char *buf = (char *)buffer;
  int32_t read_blk;
  int32_t count = 0;
  char ipAddress[SOCK_IP_SIZE];
  int port;
  unsigned int usorf_sz;
  int read_sz;
  int at_timeout;
  TickType_t xTicksToWait = RECV_TASK_TIMEOUT;
  TimeOut_t xTimeOut;
  
    //ssLoggingPrint(ESsLoggingLevel_Debug, 0, "socket_recvfrom(%d, %p, %d)",
    //               socket, buffer, length);
  
  //timer.start();
  LOCK();
  vTaskSetTimeOutState(&xTimeOut);
  
  
  while (success && (length > 0)) 
  {
    at_timeout = self->at_timeout;
    atparser_set_timeout(self->at, 1000);
    
    read_blk = MAX_READ_SIZE;
    if (read_blk > length) 
    {
      read_blk = length;
    }
    if (self->sockets[socket].pending > 0) 
    {
      ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Socket %d: has %d byte(s) pending",
                     socket, self->sockets[socket].pending);
      memset (ipAddress, 0, sizeof (ipAddress)); // Ensure terminator
      
      
      // Note: the maximum length of UDP packet we can receive comes from
      // fitting all of the following into one buffer:
      //
      // +USORF: xx,"max.len.ip.address.ipv4.or.ipv6",yyyyy,wwww,"the_data"\r\n
      //
      // where xx is the handle, max.len.ip.address.ipv4.or.ipv6 is NSAPI_IP_SIZE,
      // yyyyy is the port number (max 65536), wwww is the length of the data and
      // the_data is binary data. I make that 29 + 48 + len(the_data),
      // so the overhead is 77 bytes.
      
      //_at->debug_on(false); // ABSOLUTELY no time for debug here if you want to
      // be able to read packets of any size without
      // losing characters in UARTSerial
      if (atparser_send(self->at, "AT+USORF=%d,%d", socket, read_blk) &&
          atparser_recv(self->at, "+USORF: %*d,\"%" u_stringify(SOCK_IP_SIZE) "[^\"]\",%d,%d,\"",
                        ipAddress, &port, &usorf_sz)) 
      {
        // Must use what +USORF returns here as it may be less or more than we asked for
        if (usorf_sz > self->sockets[socket].pending) 
        {
          self->sockets[socket].pending = 0;
        }
        else
        {
          self->sockets[socket].pending -= usorf_sz;
        }
        // Note: insert no debug between _at->recv() and _at->read(), no time...
        if (usorf_sz > length) 
        {
          usorf_sz = length;
        }
        while((usorf_sz>0) && success)
        {
          read_sz = atparser_read(self->at, buf, usorf_sz);
          if (read_sz > 0) 
          {
            //address->sin_addr = pvPortMalloc(sizeof(ipAddress));
            strcpy(address->sin_addr, ipAddress);
            address->sin_port = port;
            ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, buf, read_sz, "[SOCK rd] %s:%d (%d)-> ", ipAddress, port, usorf_sz);
            count += read_sz;
            buf += read_sz;
            length -= read_sz;
            if ((usorf_sz < read_blk) || (usorf_sz == MAX_READ_SIZE))
            {
              length = 0; // If we've received less than we asked for, or
              // the max size, then a whole UDP packet has arrived and
              // this means DONE.
            }
            usorf_sz -= read_sz;
          }
          else
          {
            // read() should not fail
            success = false;
          }
          ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Socket %d: now has only %d byte(s) pending",
                         socket, self->sockets[socket].pending);
        }
        // Wait for the "OK" before continuing
        atparser_recv(self->at, "OK");
      }
      else
      {
        // Should never fail to do _at->send()/_at->recv()
        success = false;
      }
      //_at->debug_on(_debug_trace_on);
    }
    else if (xTaskCheckForTimeOut(&xTimeOut, &xTicksToWait) != pdFALSE)
    {
      //Wait for URCs
      atparser_recv(self->at, UNNATURAL_STRING);
      break;
    }
    
    atparser_set_timeout(self->at, at_timeout);
    
  }
  
  //timer.stop();
  UNLOCK();
  ulTaskNotifyTake(pdTRUE, xTicksToWait); // Merkat experimenting with this
  
  // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "socket_recvfrom: %d \"%*.*s\"", count, count, count, buf - count);
  
  return count;
}

int16_t modem_socket_connect(modem_t *self, int socket, const struct SocketAddress_in *address)
{
  bool success = true;
  
  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "socket_connect(%d, %s:%d)",
                 socket, address->sin_addr, address->sin_port);
  LOCK();
  if (atparser_send(self->at, "AT+USOCO=%d,\"%s\",%d", socket, address->sin_addr, address->sin_port) &&
      atparser_recv(self->at, "OK"))
  {
    success = false;
  }
  else
    success = true;
  
  UNLOCK();
  return success;
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/
bool power_up(modem_t *self)
{
  bool success = false;
  
  for(int i=1; !success && i<=10; i++)
  {
    hal_modem_power_up();
    osDelay(1000*i);
    success = atparser_send(self->at, "AT") && atparser_recv(self->at, "OK");
  }
  if(!success)
  {
    // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Failed to power up modem.");
  }
  else
  {
    success = atparser_send(self->at, "ATE0") && atparser_recv(self->at, "OK") &&
      atparser_send(self->at, "AT+CMEE=2") && atparser_recv(self->at, "OK");
    if(!success)
    {
      // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Preliminary modem setup failed.\n");
    }
  }
  
  return success;
}

bool reset(modem_t *self)
{
  bool success = true;;
  
  hal_modem_reset();
  
  return success;
}

bool init_sim_card(modem_t *self)
{
  bool success = false;
  bool done = false;
  
  for(int i=0; !done && i<10; i++)
  {
    char pinstr[16];
    
    if(atparser_send(self->at, "AT+CPIN?") &&
       atparser_recv(self->at, "+CPIN: %15[^\n]\n", pinstr) &&
         atparser_recv(self->at, "OK"))
    {
      done = true;
      // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "pin string: <%s>\n", pinstr);
      if(strcmp(pinstr, "SIM PIN") == 0)
      {
        self->sim_pin_enabled = true;
        if(self->pin)
        {
          if(atparser_send(self->at, "AT+CPIN=\"%s\"", self->pin))
          {
            if(atparser_recv(self->at, "OK"))
            {
              // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "pin correct\n");
              success = true;
            }
            else
            {
              // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "pin incorrect\n");
            }
          }
        }
        else
        {
          // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "no pin specified\n");
        }
      }
      else if(strcmp(pinstr, "READY") == 0)
      {
        self->sim_pin_enabled = false;
        success = true;
        // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "no pin required\n");
      }
      else
      {
        // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "unexpected response from SIM: \"%s\"\n", pinstr);
      }
    }
  }
  
  return success;
}


// Get the device ID.
bool set_device_identity(modem_t *self)
{
  char buf[20];
  bool success;
  LOCK();
  
  success = atparser_send(self->at, "ATI") && atparser_recv(self->at, "%19[^\n]\nOK\n", buf);
  
  if (success)
  {
    if (strstr(buf, "SARA-U2"))
    {
      self->dev_info.dev = DEV_SARA_U2;
    }
    else if (strstr(buf, "SARA-N2"))
    {
      self->dev_info.dev = DEV_SARA_N2;
    }
  }
  
  UNLOCK();
  return success;
}

// Send initialisation AT commands that are specific to the device.
bool device_init(modem_t *self)
{
  bool success = false;
  LOCK();
  
  
  if (self->dev_info.dev == DEV_SARA_U2)
  {
    success = atparser_send(self->at, "AT+UGPIOC=16,2") && atparser_recv(self->at, "OK");
  }
  else
  {
    success = true;
  }
  
  UNLOCK();
  return success;
}


// Active a connection profile on board the modem.
// Note: the AT interface should be locked before this is called.
bool activate_profile(modem_t *self,
                      const char* apn,
                      const char* username,
                      const char* password)
{
  bool activated = false;
  bool success = false;
  int at_timeout = self->at_timeout;
  //SocketAddress_in address;
  
  // Set up the APN
  if (apn) {
    success = atparser_send(self->at, "AT+UPSD=" PROFILE ",1,\"%s\"", apn) && atparser_recv(self->at, "OK");
  }
  if (success && username) {
    success = atparser_send(self->at, "AT+UPSD=" PROFILE ",2,\"%s\"", username) && atparser_recv(self->at, "OK");
  }
  if (success && password) {
    success = atparser_send(self->at, "AT+UPSD=" PROFILE ",3,\"%s\"", password) && atparser_recv(self->at, "OK");
  }
  
  if (success) {
    // Set up dynamic IP address assignment.
    success = atparser_send(self->at, "AT+UPSD=" PROFILE ",7,\"0.0.0.0\"") && atparser_recv(self->at, "OK");
    // Activate, waiting 30 seconds for the connection to be made
    atparser_set_timeout(self->at, 30000);
    activated = atparser_send(self->at, "AT+UPSDA=" PROFILE ",3") && atparser_recv(self->at, "OK");
    atparser_set_timeout(self->at, at_timeout);
  }
  
  return activated;
}


bool get_iccid(modem_t *self)
{
  bool success;
  LOCK();
  
  // Returns the ICCID (Integrated Circuit Card ID) of the SIM-card.
  // ICCID is a serial number identifying the SIM.
  // AT Command Manual UBX-13002752, section 4.12
  success = atparser_send(self->at, "AT+CCID") && atparser_recv(self->at, "+CCID: %20[^\n]\nOK\n", self->dev_info.iccid);
  // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "DevInfo: ICCID=%s\n", self->dev_info.iccid);
  
  UNLOCK();
  return success;
}

bool get_imsi(modem_t *self)
{
  bool success;
  LOCK();
  
  // International mobile subscriber identification
  // AT Command Manual UBX-13002752, section 4.11
  success = atparser_send(self->at, "AT+CIMI") && atparser_recv(self->at, "%15[^\n]\nOK\n", self->dev_info.imsi);
  // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "DevInfo: IMSI=%s\n", self->dev_info.imsi);
  
  UNLOCK();
  return success;
}

bool get_imei(modem_t *self)
{
  bool success;
  LOCK();
  
  // International mobile equipment identifier
  // AT Command Manual UBX-13002752, section 4.7
  success = atparser_send(self->at, "AT+CGSN") && atparser_recv(self->at, "%15[^\n]\nOK\n", self->dev_info.imei);
  // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "DevInfo: IMEI=%s\n", self->dev_info.imei);
  
  UNLOCK();
  return success;
}

bool get_meid(modem_t *self)
{
  bool success;
  LOCK();
  
  // Mobile equipment identifier
  // AT Command Manual UBX-13002752, section 4.8
  success = atparser_send(self->at, "AT+GSN") && atparser_recv(self->at, "%18[^\n]\nOK\n", self->dev_info.meid);
  // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "DevInfo: MEID=%s\n", self->dev_info.meid);
  
  UNLOCK();
  return success;
}

bool is_registered_csd(modem_t *self)
{
  return (self->dev_info.reg_status_csd == CSD_REGISTERED) ||
    (self->dev_info.reg_status_csd == CSD_REGISTERED_ROAMING) ||
      (self->dev_info.reg_status_csd == CSD_CSFB_NOT_PREFERRED);
}

bool is_registered_psd(modem_t *self)
{
  return (self->dev_info.reg_status_psd == PSD_REGISTERED) ||
    (self->dev_info.reg_status_psd == PSD_REGISTERED_ROAMING);
}

bool is_registered_eps(modem_t *self)
{
  return (self->dev_info.reg_status_eps == EPS_REGISTERED) ||
    (self->dev_info.reg_status_eps == EPS_REGISTERED_ROAMING);
}


void set_rat(modem_t *self, int AcTStatus)
{
  if(AcTStatus<RAN_TYPE_LAST)
  {
    // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Connected to: %s", ran_type_name_table[AcTStatus]);
  }
  else
  {
    // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Unknown RAT %d", AcTStatus);
  }
  
  self->dev_info.rat = (RadioAccessNetworkType)AcTStatus;
}

void set_nwk_reg_status_csd(modem_t *self, int status)
{
  switch (status) {
  case CSD_NOT_REGISTERED_NOT_SEARCHING:
  case CSD_NOT_REGISTERED_SEARCHING:
    // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Not (yet) registered for circuit switched service\n");
    break;
  case CSD_REGISTERED:
  case CSD_REGISTERED_ROAMING:
    // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Registered for circuit switched service\n");
    break;
  case CSD_REGISTRATION_DENIED:
    // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Circuit switched service denied\n");
    break;
  case CSD_UNKNOWN_COVERAGE:
    // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Out of circuit switched service coverage\n");
    break;
  case CSD_SMS_ONLY:
    // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "SMS service only\n");
    break;
  case CSD_SMS_ONLY_ROAMING:
    // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "SMS service only\n");
    break;
  case CSD_CSFB_NOT_PREFERRED:
    // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Registered for circuit switched service with CSFB not preferred\n");
    break;
  default:
    // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Unknown circuit switched service registration status. %d\n", status);
    break;
  }
  
  self->dev_info.reg_status_csd = (NetworkRegistrationStatusCsd)status;
}

// Callback for CME ERROR and CMS ERROR.
void CMX_ERROR_URC(void *param)
{
  modem_t *self = (modem_t *)param;
  char buf[48];
  
  if (read_at_to_char(self, buf, sizeof (buf), '\n') > 0) {
    // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "AT error %s\n", buf);
  }
  parser_abort_cb(self);
}

// Callback for circuit switched registration URC.
void CREG_URC(void *param)
{
  modem_t *self = (modem_t *)param;
  char buf[10];
  int status;
  
  // If this is the URC it will be a single
  // digit followed by \n.  If it is the
  // answer to a CREG query, it will be
  // a ": %d,%d\n" where the second digit
  // indicates the status
  // Note: not calling atparser_recv() from here as we're
  // already in an atparser_recv()
  if (read_at_to_char(self, buf, sizeof (buf), '\n') > 0) {
    if (sscanf(buf, ": %*d,%d", &status) == 1) {
      set_nwk_reg_status_csd(self, status);
    } else if (sscanf(buf, ": %d", &status) == 1) {
      set_nwk_reg_status_csd(self, status);
    }
  }
}

// Callback for Socket Read URC.
void UUSORD_URC(void *param)
{
  modem_t *self = (modem_t *)param;
  int nbytes;
  char buf[32];
  int socket;
  
  // Note: not calling _at->recv() from here as we're
  // already in an _at->recv()
  // +UUSORD: <socket>,<length>
  if (read_at_to_char(self, buf, sizeof (buf), '\n') > 0) {
    if (sscanf(buf, ": %d,%d", &socket, &nbytes) == 2) {
      if (socket < SOCKET_COUNT) {
        self->sockets[socket].pending = nbytes;
        // No debug prints here as they can affect timing
        // and cause data loss in UARTSerial
        //if (socket->callback != NULL) {
        //    socket->callback(socket->data);
        //}
      }
    }
  }
}

// Callback for Socket Read From URC.
void UUSORF_URC(void *param)
{
  int socket;
  int nbytes;
  char buf[32];
  modem_t *self = (modem_t *)param;
  
  // Note: not calling _at->recv() from here as we're
  // already in an _at->recv()
  // +UUSORF: <socket>,<length>
  if (read_at_to_char(self, buf, sizeof (buf), '\n') > 0) {
    if (sscanf(buf, ": %d,%d", &socket, &nbytes) == 2) {
      if (socket < SOCKET_COUNT) {
        self->sockets[socket].pending = nbytes;
        // No debug prints here as they can affect timing
        // and cause data loss in UARTSerial
        //if (socket->callback != NULL) {
        //    socket->callback(socket->data);
        //}
      }
    }
  }
}

// Callback for Socket Close URC.
void UUSOCL_URC(void *param)
{
  int socket;
  char buf[32];
  modem_t *self = (modem_t *)param;
  
  // Note: not calling _at->recv() from here as we're
  // already in an _at->recv()
  // +UUSOCL: <socket>
  if (read_at_to_char(self, buf, sizeof (buf), '\n') > 0) {
    if (sscanf(buf, ": %d", &socket) == 1) {
      // ssLoggingPrint(ESsLoggingLevel_Debug, 0, "Socket %d closed by remote host", socket);
      self->sockets[socket].state = SOCKET_CLOSED;
    }
  }
}



void parser_abort_cb(void *param)
{
  modem_t *self = param;
  atparser_abort(self->at);
}

// Read up to size bytes from the AT interface up to a "end".
// Note: the AT interface should be locked before this is called.
int read_at_to_char(modem_t *self, char * buf, int size, char end)
{
  int count = 0;
  int x = 0;
  
  if (size > 0) {
    for (count = 0; (count < size) && (x >= 0) && (x != end); count++) {
      x = atparser_getc(self->at);
      *(buf + count) = (char) x;
    }
    
    count--;
    *(buf + count) = 0;
    
    // Convert line endings:
    // If end was '\n' (0x0a) and the preceding character was 0x0d, then
    // overwrite that with null as well.
    if ((count > 0) && (end == '\n') && (*(buf + count - 1) == '\x0d')) {
      count--;
      *(buf + count) = 0;
    }
  }
  
  return count;
}
