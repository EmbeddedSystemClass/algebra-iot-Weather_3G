/**
* @file     
* @brief    
* @warning
* @details
*
* Copyright (c) Smart Sense d.o.o 2018. All rights reserved.
*
**/

/*------------------------- INCLUDED FILES ************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
  
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"

  
#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "assert.h"
#include "ssWifi.h"
#include "ssDevMan.h"
#include "ssWifiWrapper.h"
#include "ssLogging.h"
 

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/
/* Ugly but should do for testing */
uint8_t wifi_flag = 0;
netif_t  *wifi_dev;

/*------------------------- PRIVATE VARIABLES --------------------------------*/

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/
static int32_t wifi_gethostbyname(char const *hostname, uint32_t *out_ip_addr);

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

static int16_t wifi_socket_open(int16_t domain, int16_t type, int16_t protocol)
{
    int sock;
    SlTimeval_t tTimeout;
    tTimeout.tv_sec = 0;
    tTimeout.tv_usec = 10000;
    sock = sl_Socket(domain, type, protocol);
    sl_SetSockOpt(sock, SL_SOL_SOCKET, SL_SO_RCVTIMEO, &tTimeout, sizeof(SlTimeval_t));

    return sock;
}

static int16_t wifi_socket_close(netif_t *dev, int16_t socket)
{
    return sl_Close(socket);
}

static int16_t wifi_connect(netif_t *dev, int16_t socket, const struct sockaddr *addr, int16_t addrlen)
{
    int16_t result = 0;
  
    result = sl_Connect(socket, (const SlSockAddr_t *)addr, addrlen);

    return result;

}
static int16_t wifi_send(netif_t *dev, int16_t socket, const char *buf, int16_t len, int16_t flags)
{
    int16_t result = 0;

    result = sl_Send(socket, buf, len, flags);

    return result;

}
static int16_t wifi_sendto(netif_t *dev, int16_t s, char *buf, int16_t len, int16_t flags,
                                         const struct sockaddr_in *to, uint16_t to_len)
{
    int16_t result;

    ssLoggingPrint(ESsLoggingLevel_Debug, 0, "wifi_sendto 0x%x:%d", to->sin_addr.s_addr, to->sin_port);
    result = sl_SendTo(s, buf, len, flags, (const SlSockAddr_t *)to, to_len);

    return result;
}

static int16_t wifi_recv(netif_t *dev, int16_t socket, char *buf, int16_t len, int16_t flags)
{
    int16_t result = 0;

    result = sl_Recv(socket, buf, (uint8_t)len, flags);

    return result;
}
static int16_t wifi_recvfrom(netif_t *dev, int16_t s, char *buf, int16_t len, int16_t flags,
                                         struct sockaddr_in *from, uint16_t *fromlen)
{
    int16_t result = 0;

    result = sl_RecvFrom(s, buf, len, flags, (struct SlSockAddr_t *)from, (uint16_t *)fromlen);
    if(result>0)
    {
      ssLoggingPrint(ESsLoggingLevel_Debug, 0, "wifi_recvfrom 0x%x:%d (%d bytes)", from->sin_addr.s_addr, from->sin_port, result);
    }

    return result;
}
static int32_t wifi_gethostbyname(char const *hostname, uint32_t *out_ip_addr)
{
  int32_t status;
  
  status = (int32_t)sl_NetAppDnsGetHostByName((_i8 *)hostname, strlen(hostname), (_u32*)out_ip_addr, SL_AF_INET);
  //*out_ip_addr = ntohl(*out_ip_addr);
  
  return status;
}


netif_t *s_init_wifi_dev(void)
{
  if (!wifi_flag)
  {
    wifi_dev = pvPortMalloc(sizeof(netif_t));
    assert(wifi_dev);
    wifi_dev->socket_open       = &wifi_socket_open;
    wifi_dev->socket_close      = &wifi_socket_close;
    wifi_dev->socket_connect    = &wifi_connect;
    wifi_dev->socket_send       = &wifi_send;
    wifi_dev->socket_sendto     = &wifi_sendto;
    wifi_dev->socket_recv       = &wifi_recv;
    wifi_dev->socket_recvfrom   = &wifi_recvfrom;
    wifi_dev->gethostbyname     = &wifi_gethostbyname;

    initWifiModem();
    establishConnectionWithAP();
    wifi_flag = 1;
  }

    return (netif_t *)wifi_dev;  
}

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/


#ifdef __cplusplus
}
#endif
