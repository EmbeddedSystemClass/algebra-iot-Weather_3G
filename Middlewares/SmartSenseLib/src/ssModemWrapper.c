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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
  
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
//#include "ssSocket.h"
#include "assert.h"
#include "ssDevMan.h"
#include "ATCmdParser.h"
#include "ssModem.h"
#include "ssModemWrapper.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/
#define APN       "internet.ht.hr"
//#define APN       "data.vip.hr"

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/
uint8_t modem_flag = 0;
modem_t *modem;
/*------------------------- PRIVATE VARIABLES --------------------------------*/

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

static int16_t mdm_socket_open(int16_t domain, int16_t type, int16_t protocol)
{

    return modem_socket_open(modem, protocol);
}


static int16_t mdm_socket_close(netif_t *dev, int16_t socket)
{

    return modem_socket_close((modem_t *)dev, socket);
}

static int16_t mdm_connect(netif_t *dev, int16_t socket, const struct sockaddr *addr, int16_t addrlen)
{
    int16_t result = 0;
    struct SocketAddress_in address;

    address.sin_family = ((struct sockaddr_in *)addr)->sin_family;
    address.sin_port = ((struct sockaddr_in *)addr)->sin_port;
    strcpy(address.sin_addr, addr->sa_data);
  
    result = modem_socket_connect((modem_t *)dev, socket, &address);

    return result;

}
static int16_t mdm_send(netif_t *dev, int16_t socket, const char *buf, int16_t len, int16_t flags)
{
    int16_t result = 0;

    result = modem_socket_send((modem_t *)dev, socket, buf, len);

    return result;

}
static int16_t mdm_sendto(netif_t *dev, int16_t s, char *buf, int16_t len, int16_t flags,
                                         const struct sockaddr_in *to, uint16_t to_len)
{
    int16_t result;
    struct SocketAddress_in address;
    uint32_t addr = htonl(to->sin_addr.s_addr);

    address.sin_family = to->sin_family;
    address.sin_port = htons(to->sin_port);
    inet_ntop(AF_INET, &addr, address.sin_addr, INET_ADDRSTRLEN);

    result = modem_socket_sendto((modem_t *)dev, s, buf, len, &address);

    return result;
}

static int16_t mdm_recv(netif_t *dev, int16_t socket, char *buf, int16_t len, int16_t flags)
{
    int16_t result = 0;

    result = modem_socket_recv((modem_t *)dev, socket, buf, (uint8_t)len);

    return result;
}
static int16_t mdm_recvfrom(netif_t *dev, int16_t s, char *buf, int16_t len, int16_t flags,
                                         struct sockaddr_in *from, uint16_t *fromlen)
{
    int16_t result = 0;
    struct SocketAddress_in address;

    result = modem_socket_recvfrom((modem_t *)dev, s, buf, len, &address);

    //address.sin_family = AF_INET;
    from->sin_family = address.sin_family;
    from->sin_port = htons(address.sin_port);
    inet_aton(address.sin_addr, &from->sin_addr);

    return result;
}
static int32_t mdm_gethostbyname(char const *hostname, uint32_t *out_ip_addr)
{
  return modem_gethostbyname(modem, hostname, out_ip_addr);
}

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
void *s_init_modem(void)
{
  if (!modem_flag)
  {
    modem = modem_create();
    
    modem->com_dev.socket_open     = mdm_socket_open;
    modem->com_dev.socket_close    = mdm_socket_close;
    modem->com_dev.socket_connect  = mdm_connect;
    modem->com_dev.socket_send     = mdm_send;
    modem->com_dev.socket_sendto   = mdm_sendto;
    modem->com_dev.socket_recv     = mdm_recv;
    modem->com_dev.socket_recvfrom = mdm_recvfrom;
    modem->com_dev.gethostbyname   = mdm_gethostbyname;
    assert(modem);
    assert(modem_init(modem, NULL));
    assert(modem_nwk_register(modem));
    modem_set_credentials(modem, APN, NULL, NULL);
    assert(modem_nwk_connect(modem));
    modem_flag = 1;
  }

  return (void *)modem;
}

