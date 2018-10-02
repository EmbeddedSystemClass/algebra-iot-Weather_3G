/**
 * @file
 * @brief
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef SS_SOCKET_H
#define SS_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

#define NUM_SOCKETS    7
#define MAX_SOCKET_ADDRESS 16

/*#define SOCK_STREAM                         (1)                       TCP Socket                                                          */
/*#define SOCK_DGRAM                          (2)                       UDP Socket                                                          */
/*#define SOCK_RAW                            (3)                       Raw socket                                                          */
/*#define IPPROTO_TCP                         (6)                       TCP Raw Socket                                                      */
/*#define IPPROTO_UDP                         (17)                      UDP Raw Socket                                                      */
/*#define IPPROTO_RAW                         (255)                     Raw Socket                                                          */


    



#define netbuf_len(buf)              ((buf)->p->tot_len)

#define MEM_ALIGNMENT                    1
#ifndef MEM_ALIGN_SIZE
#define MEM_ALIGN_SIZE(size) (((size) + MEM_ALIGNMENT - 1) & ~(MEM_ALIGNMENT-1))
#endif

#define MIN(x , y)  (((x) < (y)) ? (x) : (y))

#ifndef MEM_ALIGN
#define MEM_ALIGN(addr) ((void *)(((uint32_t)(addr) + MEM_ALIGNMENT - 1) & ~(uint32_t)(MEM_ALIGNMENT-1)))
#endif

/**
 * TCP_MSS: TCP Maximum segment size. (default is 536, a conservative default,
 * you might want to increase this.)
 * For the receive side, this MSS is advertised to the remote side
 * when opening a connection. For the transmit size, this MSS sets
 * an upper limit on the MSS advertised by the remote host.
 */
#ifndef TCP_MSS
#define TCP_MSS                         536
#endif

#define MEMP_PBUF_POOL                  10
#define PBUF_LINK_HLEN                  14

/**
 * PBUF_POOL_BUFSIZE: the size of each pbuf in the pbuf pool. The default is
 * designed to accomodate single full size TCP frame in one pbuf, including
 * TCP_MSS, IP header, and link header.
 */
#ifndef PBUF_POOL_BUFSIZE
#define PBUF_POOL_BUFSIZE               MEM_ALIGN_SIZE(TCP_MSS+40+PBUF_LINK_HLEN)
#endif

#define PBUF_POOL_BUFSIZE_ALIGNED MEM_ALIGN_SIZE(PBUF_POOL_BUFSIZE)

/*------------------------- TYPE DEFINITIONS ---------------------------------*/


typedef uint16_t address_family_t;
typedef unsigned int flag_t;

#define HTONL(long_var)    ((((long_var) & 0x000000FFU) << 24U) | (((long_var) & 0x0000FF00U) << 8U) | \
                            (((long_var) & 0x00FF0000U) >> 8U) | (((long_var) & 0xFF000000U) >> 24U))
#define HTONS(short_var)   (short_var)
#define MULTICAST_SOCKET_MAX (3)

#define PBUF_TRANSPORT_HLEN 20
#define PBUF_IP_HLEN        20

/**************************************************************************/ /*!
 * @brief General return codes, used by most of API functions.
 ******************************************************************************/
typedef enum
{
    STATUS_OK  = (0), /**< No error.*/
    STATUS_ERR = (-1) /**< There is error.*/
} err_return_t;
/**************************************************************************/ /*!
 * @brief Boolean type.
 ******************************************************************************/
typedef enum
{
    SS_FALSE = 0, /**< FALSE Boolean value.*/
    SS_TRUE  = 1  /**< TRUE Boolean value.*/
} bool_t;



/**************************************************************************/ /*!
 * @brief Socket state.
 ******************************************************************************/
typedef enum
{
    SS_UNCONNECTED = (0),   /**< @brief Not connected to any socket.*/
    SS_CONNECTING  = (1),   /**< @brief In process of connecting.*/
    SS_CONNECTED   = (2),   /**< @brief Connected to a socket.*/
    SS_LISTENING   = (3)    /**< @brief In listening state.*/
} socket_state_t;

/**************************************************************************/ /*!
 * @brief Socket types.
 ******************************************************************************/
typedef enum
{
    SS_SOCK_UNSPEC = (0U),  /**< @brief Unspecified socket type.
                         */
    SS_SOCK_STREAM = (1U),  /**< @brief Stream socket.@n
                         * Provides reliable, two-way, connection-based
                         * byte stream. It corresponds to the TCP protocol
                         */
    SS_SOCK_DGRAM  = (2U),  /**< @brief Datagram socket.@n
                         * Provides unreliable, connectionless datagrams.
                         * It corresponds to the UDP protocol.
                         */
    SS_SOCK_RAW    = (3U)   /**< @brief Raw socket.@n
                         * Raw sockets allow an application to have direct access to
                         * lower-level communication protocols.
                         * Raw sockets are intended to take advantage of some protocol feature
                         * that is not directly accessible through a normal interface,
                         * or to build new protocols on top of existing low-level protocols.@n
                         * It can be enabled by the @ref FNET_CFG_RAW option.
                         */
} socket_type_t;

/**************************************************************************/ /*!
 * @brief Protocol numbers and Level numbers for the @ref fnet_socket_setopt()
 * and the @ref fnet_socket_getopt().
 ******************************************************************************/
typedef enum
{
    SS_IPPROTO_IP      = (0), /**< @brief IPv4 options level number
                            *   for @ref fnet_socket_getopt() and @ref fnet_socket_setopt().*/
    SS_IPPROTO_ICMP    = (1), /**< @brief ICMPv4 protocol number.*/
    SS_IPPROTO_IGMP    = (2), /**< @brief IGMP protocol number.*/
    SS_IPPROTO_TCP     = (6), /**< @brief TCP protocol number; TCP options level number
                            *   for @ref fnet_socket_getopt() and @ref fnet_socket_setopt().*/
    SS_IPPROTO_UDP     = (17),/**< @brief UDP protocol number.*/
    SS_IPPROTO_IPV6    = (41), /**< @brief IPv6 options level number
                            *    for @ref fnet_socket_getopt() and @ref fnet_socket_setopt().*/
    SS_IPPROTO_ICMPV6  = (58),/**< @brief ICMPv6 protocol number.*/
    SS_SOL_SOCKET      = (255255)  /**< @brief Socket options level number for
                                 * @ref fnet_socket_getopt() and @ref fnet_socket_setopt().*/
} protocol_t;

typedef enum { 
  PBUF_TRANSPORT, PBUF_IP, PBUF_LINK, PBUF_RAW_TX, 
  PBUF_RAW 
}pbuf_layer_t;

typedef enum {
  PBUF_RAM, /* pbuf data is stored in RAM */
  PBUF_ROM, /* pbuf data is stored in ROM */
  PBUF_REF, /* pbuf comes from the pbuf pool */
  PBUF_POOL /* pbuf payload refers to RAM */
} pbuf_type_t;



struct pbuf_t {
  /* next pbuf in singly linked pbuf chain */
  struct pbuf_t *next;
  /* pointer to the actual data in the buffer */
  void *payload;
  /* total length of this buffer and all next buffers in chain
  belonging to the same packet */
  uint16_t tot_len;
  /* length of this buffer */
  uint16_t len;
  /* pbuf type as uin8_t instead of enum to save space */
  uint8_t type;
  /* misc flags */
  uint8_t flags;
  /**
   * the reference count always equals the number of pointers
   * that refer to this pbuf. This can be pointers from an application,
   * the stack itself, or pbuf->next pointers from a chain.
   */
  uint16_t ref;
};

struct netbuf_t {
  struct pbuf_t   *p, *ptr;
  char     *addr;
#if NETBUF_RECVINFO
  struct ip_addr *toaddr;
  u16_t toport;
#endif /* NETBUF_RECVINFO */
};


/************************************************************************
*    Structure per socket.
*************************************************************************/
struct socket_t {
  /* Socket state */
  socket_state_t state;
  /* Data that was left from the previous read */
  struct netbuf_t *lastdata;
  /* Offset in the data that was left from the previous read */
  uint16_t lastoffset;
  /* Number of times data was received, set by event_callback() */
  int16_t rcevent;
  /* Number of times data was sent, set by event_callback() */
  uint16_t sendevent;
  /* Socket flags */
  uint16_t flags;
  /* Last error that occured on this socket */
  int8_t err;
  /* Status of socket */
  uint8_t taken;
  protocol_t protocol;
  socket_type_t type;
  struct netif_t *interface;
};


/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
int16_t socket(int domain, int type, int protocol);
err_return_t bind(int s, const struct sockaddr *name, uint16_t namelen);
int32_t connect(int s, const struct sockaddr *name, socklen_t namelen);
err_return_t listen(int s, uint16_t backlog);
int8_t accept(int  s, struct sockaddr *addr, uint16_t *addrlen);
int32_t send(int s, const void *buf, size_t len, int8_t flags);
int32_t sendto(int s, const void *data, size_t size, int8_t flags, const struct sockaddr *to, socklen_t tolen);
int32_t recv(int s, void *data, size_t len, int8_t flags);
int32_t recvfrom(int s, void *mem, size_t len, int8_t flags, struct sockaddr *from, socklen_t *from_len);
int8_t close(int16_t s);
int s_getaddrinfo(const char *nodename, const char *port, const struct addrinfo *hints, struct addrinfo **res);



/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* SS_SOCKET_H */
