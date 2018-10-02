/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _NETINET_IN_H
#define _NETINET_IN_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
/*
 * Protocols (RFC 1700)
 */
#define IPPROTO_IP              0               /* dummy for IP */
#define IPPROTO_UDP             17              /* user datagram protocol */
#define IPPROTO_TCP             6               /* tcp */

#define IPPROTO_IPV6		        41
#define IPV6_V6ONLY             23  /**< \brief IPv6 only -- no IPv4 (get/set) */

#define INET_ADDRSTRLEN         16
#define INET6_ADDRSTRLEN        46

#define INET6_ADDRESS_SIZE        16
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/
typedef uint16_t in_port_t;
typedef uint32_t in_addr_t;

struct in_addr 
{
  in_addr_t s_addr;  
};

struct sockaddr_in
{
  sa_family_t     sin_family;     /* AF_INET */ 
  in_port_t       sin_port;       /* Port number */
  struct in_addr  sin_addr;       /* IP address */
  char            sin_zero[8];    /* not used */  
};

struct in6_addr 
{
  unsigned char   s6_addr[INET6_ADDRESS_SIZE];   /* IPv6 address */
};

struct sockaddr_in6 {
  sa_family_t     sin6_family;   /* AF_INET6 */
  in_port_t       sin6_port;     /* port number */
  uint8_t        sin6_flowinfo; /* IPv6 flow information */
  struct in6_addr sin6_addr;     /* IPv6 address */
};

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _NETINET_IN_H */