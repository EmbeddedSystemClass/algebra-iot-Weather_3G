/**
 * @file     
 * @brief    
 * @warning
 * @details
 *
 * Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
 *
 **/

#ifndef _SYS_SOCKET_H
#define _SYS_SOCKET_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*
 * Definitions related to sockets: types, address families, options.
 */

/*
 * Types
 */
#define	SOCK_STREAM	1		/* stream socket */
#define	SOCK_DGRAM	2		/* datagram socket */
#define	SOCK_RAW	3		/* raw-protocol interface */
#define	SOCK_RDM	4		/* reliably-delivered message */
#define	SOCK_SEQPACKET	5		/* sequenced packet stream */
  
/*
 * Address families.
 */
#define	 AF_UNSPEC	            0		/* unspecified */
#define	 AF_UNIX		            1		/* local to host (pipes, portals) */
#define  AF_INET                2   /* internetwork: UDP, TCP, etc. */
#define  AF_INET6               23  /* internetwork: UDP, TCP, etc. */

/*
 * Option flags per-socket.
 */
#define	SO_DEBUG	      0x0001    /* turn on debugging info recording */
#define	SO_ACCEPTCONN	  0x0002		/* socket has had listen() */
#define	SO_REUSEADDR	  0x0004		/* allow local address reuse */
#define	SO_KEEPALIVE	  0x0008		/* keep connections alive */
#define	SO_DONTROUTE	  0x0010		/* just use interface addresses */
#define	SO_BROADCAST	  0x0020		/* permit sending of broadcast msgs */
#define	SO_USELOOPBACK	0x0040		/* bypass hardware when possible */
#define	SO_LINGER	      0x0080	  /* linger on close if data present */
#define	SO_OOBINLINE	  0x0100		/* leave received OOB data in line */

/*
 * Additional options, not kept in so_options.
 */
#define SO_SNDBUF	0x1001		/* send buffer size */
#define SO_RCVBUF	0x1002		/* receive buffer size */
#define SO_SNDLOWAT	0x1003		/* send low-water mark */
#define SO_RCVLOWAT	0x1004		/* receive low-water mark */
#define SO_SNDTIMEO	0x1005		/* send timeout */
#define SO_RCVTIMEO	0x1006		/* receive timeout */
#define	SO_ERROR	0x1007		/* get error status and clear */
#define	SO_TYPE		0x1008		/* get socket type */

/*
 * Level number for (get/set)sockopt() to apply to socket itself.
 */
#define	SOL_SOCKET	0xffff		/* options for socket level */
  
#define	MSG_OOB		    0x1		/* process out-of-band data */
#define	MSG_PEEK	    0x2		/* peek at incoming message */
#define	MSG_DONTROUTE	0x4		/* send without using routing tables */
#define	MSG_EOR		    0x8		/* data completes record */
#define	MSG_TRUNC	    0x10		/* data discarded before delivery */
#define	MSG_CTRUNC	  0x20		/* control data lost before delivery */
#define	MSG_WAITALL	  0x40		/* wait for full request or error */
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

typedef uint32_t sa_family_t;
typedef uint32_t socklen_t;
  
struct sockaddr 
{
	uint8_t	sa_len;			  /* total length */
	uint8_t	sa_family;		/* address family */
	char	sa_data[16];		/* address value */
};
  

/*
 * Structure used for manipulating linger option.
 */
struct	linger {
	int	l_onoff;		/* option on/off */
	int	l_linger;		/* linger time */
};

/*
 * Message header for recvmsg and sendmsg calls.
 * Used value-result for recvmsg, value only for sendmsg.
 */
struct msghdr 
{
	void*	msg_name;		/* optional address */
	uint32_t	msg_namelen;		/* size of address */
	struct	iovec *msg_iov;		/* scatter/gather array */
	uint32_t	msg_iovlen;		/* # elements in msg_iov */
	void*	msg_control;		/* ancillary data, see below */
	uint32_t	msg_controllen;		/* ancillary data buffer len */
	int32_t	msg_flags;		/* flags on received message */
};

struct addrinfo 
{
  int             ai_flags;
  int             ai_family;
  int             ai_socktype;
  int             ai_protocol;
  size_t          ai_addrlen;
  struct sockaddr *ai_addr;
  char            *ai_canonname;
  struct addrinfo  *ai_next;
};

#define AI_PASSIVE 0x01

struct sockaddr_storage {
  sa_family_t   ss_family;
  uint16_t    sin_port; 
  sa_family_t	sa_len;			
  char	sa_data[16];		
};

/*------------------------- PUBLIC VARIABLES ---------------------------------*/

/*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/

void freeaddrinfo(struct addrinfo *res);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

#ifdef __cplusplus
}
#endif

#endif /* _SYS_SOCKET_H */