/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2018. All rights reserved.
 *
 **/

#ifndef __SS__MODEMWRAPPER_H
#define __SS__MODEMWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
int16_t mdm_socket_open(int16_t domain, int16_t type, int16_t protocol);
int16_t mdm_socket_close(netif_t *dev, int16_t socket);
int16_t mdm_connect(netif_t *dev, int16_t socket, const struct sockaddr *addr, int16_t addrlen);
int16_t mdm_send(netif_t *dev, int16_t socket, const char *buf, int16_t len, int16_t flags);
int16_t mdm_sendto(netif_t *dev, int16_t s, char *buf, int16_t len, int16_t flags,
                                         const struct sockaddr_in *to, uint16_t to_len);

int16_t mdm_recv(netif_t *dev, int16_t socket, char *buf, int16_t len, int16_t flags);
int16_t mdm_recvfrom(netif_t *dev, int16_t s, char *buf, int16_t len, int16_t flags,
                                         struct sockaddr_in *from, uint16_t *fromlen);

int32_t mdm_gethostbyname(char const *hostname, uint32_t *out_ip_addr);
/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __SS__MODEMWRAPPER_H */
 
