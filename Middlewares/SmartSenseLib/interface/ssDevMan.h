/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2018. All rights reserved.
 *
 **/

#ifndef __SS_DEVMAN_H
#define __SS_DEVMAN_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef struct netif_t netif_t;
struct netif_t
{
    int16_t (*socket_open)(int16_t domain, int16_t type, int16_t protocol);
    int16_t (*socket_close)(netif_t *dev, int16_t s);
    int16_t (*socket_connect)(netif_t *dev, int16_t socket, const struct sockaddr *addr, int16_t addrlen);
    int16_t (*socket_send)(netif_t *dev, int16_t socket, const char *buf, int16_t len, int16_t flags);
    int16_t (*socket_sendto)(netif_t *dev, int16_t s, char *buf, int16_t len, int16_t flags,
                                         const struct sockaddr_in *to, uint16_t to_len);

    int16_t (*socket_recv)(netif_t *dev, int16_t socket, char *buf, int16_t len, int16_t flags);
    int16_t (*socket_recvfrom)(netif_t *dev, int16_t s, char *buf, int16_t len, int16_t flags,
                                         struct sockaddr_in *from, uint16_t *fromlen);
    int32_t (*gethostbyname)(const char *hostname, uint32_t *out_ip_addr);
};
/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
netif_t *get_device(void);
//netif_t *s_init_wifi_dev(void);
//netif_t *s_init_modem(void);
/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __SS_DEVMAN_H */
 
