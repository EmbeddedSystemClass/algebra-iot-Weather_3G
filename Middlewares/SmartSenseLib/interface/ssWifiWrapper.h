/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2018. All rights reserved.
 *
 **/

#ifndef __SS_WIFIWRAPPER_H
#define __SS_WIFIWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif


#include <stdint.h>
#include "FreeRTOS.h"
#include "bsp.h"
#include "ssSocket.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

int16_t wifi_socket_open(int16_t domain, int16_t type, int16_t protocol);
int16_t wifi_socket_close(netif_t *dev, int16_t socket);
int16_t wifi_connect(netif_t *dev, int16_t socket, const struct sockaddr *addr, int16_t addrlen);
int16_t wifi_send(netif_t *dev, int16_t socket, const char *buf, int16_t len, int16_t flags);
int16_t wifi_sendto(netif_t *dev, int16_t s, char *buf, int16_t len, int16_t flags,
                                         const struct sockaddr_in *to, uint16_t to_len);

int16_t wifi_recv(netif_t *dev, int16_t socket, char *buf, int16_t len, int16_t flags);
int16_t wifi_recvfrom(netif_t *dev, int16_t s, char *buf, int16_t len, int16_t flags,
                                         struct sockaddr_in *from, uint16_t *fromlen);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __SS_WIFIWRAPPER_H */
 
