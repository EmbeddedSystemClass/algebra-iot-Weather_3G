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
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
  
#include "sys/socket.h"  
#include "netinet/in.h"
#include "arpa/inet.h"
#include "sys/errno.h"

#include "FreeRTOS.h"
#include "cmsis_os.h"
#include "assert.h"
#include "ssDevMan.h"
#include "ssSocket.h" 
#include "ssLogging.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/
#define MODEM_SOCKET      1
#define WIFI_SOCKET       2

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/
/* The global array of available sockets */
struct socket_t sockets[NUM_SOCKETS];

/*------------------------- PRIVATE VARIABLES --------------------------------*/

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/
static struct addrinfo *allocaddrinfo(void);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/

static struct socket_t *get_socket(int16_t s)
{
  struct socket_t *sock;

  if ((s < 0) || (s >= NUM_SOCKETS)) {
    return NULL;
  }

  sock = &sockets[s];

  if (!sock->taken) {

    return sock;
    //return NULL;      // MERKAT dbg
  }

  return sock;
}

/* Create a socket and return the descriptor to the app */
int16_t socket(int domain, int type, int protocol)
{
  int8_t i;
  netif_t *net_dev;
  
  net_dev = get_device();
  i = net_dev->socket_open(domain, type, protocol);
  
  if(i > -1 && !sockets[i].taken) 
  {
    ssLoggingPrint(ESsLoggingLevel_Debug, 0, "socket created id=%d (domain=%d, type=%d, protocol=%d)", i, domain, type, protocol);
    
    sockets[i].state      = SS_UNCONNECTED;
    sockets[i].lastdata   = NULL;
    sockets[i].lastoffset = 0;
    sockets[i].rcevent    = 0;
    sockets[i].sendevent  = 1;  /* TCP send buf is empty */
    sockets[i].flags      = 0;
    sockets[i].err        = 0;
    sockets[i].taken      = 1;
    sockets[i].protocol   = protocol;
    sockets[i].type       = type;
    sockets[i].interface  = net_dev;
    return i;
  }
  
  return -1;
}

int32_t connect(int s, const struct sockaddr *name, socklen_t namelen)
{
  uint8_t result = -1;
  struct socket_t *sock;

  sock = get_socket(s);
  if (!sock)
  {
    return result;
  }
  /* Will user later, junk for now */
  /*
  if (sock->state == SS_LISTENING)
  {
    return result;
  }
  if (sock->state == SS_CONNECTED)
  {
    return result;
  }
  */

  result = sock->interface->socket_connect(sock->interface, s, name, namelen);
  
  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "socket connect id=%d, name=%s, status=%s", s, name->sa_data, result);  

  if (result)
  {
    sock->state = SS_CONNECTED;
  }
  return result;
}

int32_t sendto(int s, const void *data, size_t size, int8_t flags, const struct sockaddr *to, socklen_t tolen)
{
  struct socket_t *sock;
  int32_t     result;

  sock = get_socket(s);
  if (!sock)
    return -1;

  result = sock->interface->socket_sendto(sock->interface, s, (char *)data, size, 0, (const struct sockaddr_in *)to, tolen);
  ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, data, size, "socket sendto s=%d to=0x%x:%d =>", s, ((const struct sockaddr_in *)to)->sin_addr.s_addr, ((const struct sockaddr_in *)to)->sin_port);
  
  return result;
}

int32_t send(int s, const void *data, size_t size, int8_t flags)
{
  struct socket_t *sock;
  int8_t     result = -1;

  sock = get_socket(s);
  if (!sock)
    return result;

  if (sock->state == SS_CONNECTED)
  {
      result = sock->interface->socket_send(sock->interface, s, data, size, 0);
  }

  return result;
}

int32_t recvfrom(int s, void *mem, size_t len, int8_t flags, struct sockaddr *from, socklen_t *from_len)
{
  struct socket_t   *sock;
  int32_t           length = -1;
  
  sock = get_socket(s);
  if(!sock)
  {
    return -1;
  }
      
  /* No data was left from previous operation, try to get some from the network */
  //length = sock->interface->socket_recvfrom(sock->interface, s, (buf->p->payload), len, 0, (struct sockaddr_in *)from, (uint16_t *)from_len);
  uint16_t temp_len = sizeof(struct sockaddr_in);
  length = sock->interface->socket_recvfrom(sock->interface, s, mem, len, 0, (struct sockaddr_in *)from, &temp_len);
 
  if(length>0)
  {
    ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, mem, length, "socket recvfrom s=%d from=0x%x:d =>", s, ((const struct sockaddr_in *)from)->sin_addr.s_addr, ((const struct sockaddr_in *)from)->sin_port);
  }
  
  return length;
}

int32_t recv(int s, void *mem, size_t len, int8_t flags)
{
  struct socket_t   *sock;
  int32_t           length = -1;

  sock = get_socket(s);
  if(!sock)
  {
    return -1;
  }

  length = sock->interface->socket_recv(sock->interface, s, mem, len, 0);

  return length;
}

int8_t close (int16_t s)
{
  struct socket_t *sock;
  
  sock = get_socket(s);
  if(!sock)
  {
    return -1;
  }
  else
  {
    sock->interface->socket_close(sock->interface, s);

    sock->lastdata   = NULL;
    sock->lastoffset = 0;
    sock->taken      = 0;
    sock->state      = SS_UNCONNECTED;
    vPortFree(sock);
    return 0;
  }
}

/* @TODO: not finished - DO NOT USE! */
struct hostent *gethostbyname(const char *hostname)
{
  netif_t *net_dev;
  uint32_t ipaddr;
  struct hostent *data;
  
  if(!hostname)
  {
    return NULL;
  }
  
  data = pvPortMalloc(sizeof(struct addrinfo));
  if(data)
  {
    struct in_addr addr;
    const char *p;
    
    /* Check for all-numeric hostname with no trailing dot. */
    if (isdigit(hostname[0])) 
    {
      p = hostname;
      while (*p && (isdigit(*p) || *p == '.'))
      {
        p++;
      }
      
      if (!*p && p[-1] != '.') 
      {
        /* Looks like an IP address; convert it. */
        if (inet_aton(hostname, &addr) == 0) 
        {
          /* invalid ip address */
          return NULL;
        }
        /* @TODO: fill hostent */
        return data;
      }
    }
    
    net_dev = get_device();
    net_dev->gethostbyname(hostname, &ipaddr);
    /* @TODO: fill hostent */
  }
  
  return data;
}

/* Merkat -- still working on it */
int s_getaddrinfo(const char *node, const char *service,
       const struct addrinfo *hints, struct addrinfo **res)
{
  struct in_addr addr;
  struct addrinfo *ai = NULL;
  netif_t *net_dev = NULL;
  bool is_numeric = false;
  bool is_found = false;


  if (res == NULL)
  {
    return -1;
  }
  *res = NULL;

  if ((node == NULL) && (service == NULL)) 
  {
    return -1;
  }

  /* Check for all-numeric hostname with no trailing dot.
   * No need to query network device in that case, just copy to output structure.
   */
  if (isdigit(node[0])) 
  {
    const char *p = node;
    while (*p && (isdigit(*p) || *p == '.'))
    {
      p++;
    }
    
    if (!*p && p[-1] != '.') 
    {
      /* Looks like an IP address; convert it. */
      if (inet_aton(node, &addr) != 0) 
      {
        is_numeric = true;
        is_found = true;
      }
    }
  }
  
  if((!is_found) && (!is_numeric))
  {
    /* try network device */
    net_dev = get_device();
    if(net_dev->gethostbyname(node, &addr.s_addr) == 0)
    {
      is_found = true;
    }
  }
    
  if(is_found)
  {
    ai = allocaddrinfo();
    if(ai)
    {
      ai->ai_family = AF_INET;
      if (hints != NULL) 
      {
        /* copy socktype & protocol from hints if specified */
        ai->ai_socktype = hints->ai_socktype;
        ai->ai_protocol = hints->ai_socktype; /* Merkat also waakama TODO */
      }
      ai->ai_addr->sa_family = AF_INET;
      ai->ai_addr->sa_len = sizeof(struct sockaddr);
      if(is_numeric)
      {
        strcpy(ai->ai_addr->sa_data, node);
      }
      else
      {
        if(inet_ntop(AF_INET, &addr, ai->ai_addr->sa_data, sizeof(ai->ai_addr->sa_data)) != ai->ai_addr->sa_data)
        {
          freeaddrinfo(ai);
          ai = NULL;
        }
      }
    }
  }

  *res = ai;

  return 0;
}


void freeaddrinfo(struct addrinfo *res)
{
  vPortFree(res);
}

/* Convert string to integer */
int32_t ss_atoi(const char *str)
{
  int res = 0;
  int i = 0;

  for(; str[i] != '\0'; ++i)
  {
    res = res*10 + str[i] - '0';
  }
  return res;
}


/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/
static struct addrinfo *allocaddrinfo()
{
  void *res = NULL;
  struct addrinfo *ai = NULL;
  struct sockaddr *sa = NULL;
  
  res = pvPortMalloc(sizeof(struct addrinfo) + sizeof(struct sockaddr));
  if(res)
  {
    memset(res, 0, sizeof(struct addrinfo) + sizeof(struct sockaddr));
    ai = (struct addrinfo *)res;
    sa = (struct sockaddr *)((uint8_t *)res + sizeof(struct addrinfo));
    
    ai->ai_addr = sa;
    /* Canonname not supported */
    ai->ai_canonname = NULL;
    /* Only single address per host is supported */
    ai->ai_next = NULL;
  }
 
  return ai;
}
