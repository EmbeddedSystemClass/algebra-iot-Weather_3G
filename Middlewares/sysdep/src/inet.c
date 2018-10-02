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

#include <stdlib.h>
#include <stdint.h>

#include "sys/errno.h"
#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"

/*------------------------- MACRO DEFINITIONS --------------------------------*/

/*------------------------- TYPE DEFINITIONS ---------------------------------*/

/*------------------------- PUBLIC VARIABLES ---------------------------------*/
extern int errno;

/*------------------------- PRIVATE VARIABLES --------------------------------*/

/*------------------------- PRIVATE FUNCTION PROTOTYPES ----------------------*/
static const char *inet_ntop_v4 (const void *src, char *dst, size_t size);

/*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
int inet_aton(const char *cp, struct in_addr *addr)
{
  uint32_t val;
  uint8_t base;
  char c;
  uint32_t parts[4];
  uint32_t *pp = parts;
  
  c = *cp;
  for (;;)
  {
    if (c < '0' && c > '9')
      return 0;
    val = 0;
    base = 10;
    
    if (c == '0')
    {
      c = *++cp;
    }
    for (;;)
    {
      if (c >= '0' && c <= '9')
      {
        val = (val * base) + (int)(c - '0');
        c = *++cp;
      }
      else
        break;
    }
    if (c == '.')
    {
      if (pp >= parts + 3)
        return 0;
      
      *pp++ = val;
      c = *++cp;
    }
    else
      break;
  }
  
  if (c != '\0' && c != ' ')
    return 0;
  
  /*
  * Concoct the address according to
  * the number of parts specified.
  */
  switch (pp - parts + 1) {
    
  case 0:
    return (0);       /* initial nondigit */
    
  case 1:             /* a -- 32 bits */
    break;
    
  case 2:             /* a.b -- 8.24 bits */
    if (val > 0xffffffUL)
      return (0);
    val |= parts[0] << 24;
    break;
    
  case 3:             /* a.b.c -- 8.8.16 bits */
    if (val > 0xffff)
      return (0);
    val |= (parts[0] << 24) | (parts[1] << 16);
    break;
    
  case 4:             /* a.b.c.d -- 8.8.8.8 bits */
    if (val > 0xff)
      return (0);
    val |= (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8);
    break;
  }
  
  if (addr)
    addr->s_addr = htonl(val);
  
  return 1;
}

const char *inet_ntop(int af, const void *src, char *dst, size_t size)
{
  switch (af) 
	{
  case AF_INET :
	  return inet_ntop_v4 (src, dst, size);
  default :
    errno = EAFNOSUPPORT;
    return NULL;
  }
}


/* Convert from host order to network long byte order */ 
uint32_t htonl(uint32_t hostlong)
{
  uint32_t i = 1; 
  int8_t *p = (int8_t *)&i;  
  if (p[0] == 1) /* little endian */
  {
    p[0] = ((int8_t* )&hostlong)[3];
    p[1] = ((int8_t* )&hostlong)[2];
    p[2] = ((int8_t* )&hostlong)[1];
    p[3] = ((int8_t* )&hostlong)[0];
    return i;
  }
  else /* big endian */
  {
    return hostlong; 
  }
}

uint32_t ntohl(uint32_t netlong)
{
  uint32_t i = 1; 
  int8_t *p = (int8_t *)&i;  
  if (p[0] == 1) /* little endian */
  {
    p[0] = ((int8_t* )&netlong)[3];
    p[1] = ((int8_t* )&netlong)[2];
    p[2] = ((int8_t* )&netlong)[1];
    p[3] = ((int8_t* )&netlong)[0];
    return i;
  }
  else /* big endian */
  {
    return netlong; 
  }
}

/* Convert from host order to network short byte order */ 
uint16_t htons(uint16_t hostshort)
{
  int16_t i = 1; 
  int8_t *p = (int8_t *)&i;  
  if (p[0] == 1) /* little endian */
  {
    p[0] = ((int8_t* )&hostshort)[1];
    p[1] = ((int8_t* )&hostshort)[0];
    return (uint16_t)i;
  }
  else /* big endian */
  {
    return hostshort; 
  }
}

uint16_t ntohs(uint16_t netshort)
{
  int16_t i = 1; 
  int8_t *p = (int8_t *)&i;  
  if (p[0] == 1) /* little endian */
  {
    p[0] = ((int8_t* )&netshort)[1];
    p[1] = ((int8_t* )&netshort)[0];
    return (uint16_t)i;
  }
  else /* big endian */
  {
    return netshort; 
  }
}

/*------------------------- PRIVATE FUNCTION DEFINITIONS ---------------------*/

static const char *inet_ntop_v4 (const void *src, char *dst, size_t size)
{
    const char digits[] = "0123456789";
    int i;
    struct in_addr *addr = (struct in_addr *)src;
    unsigned long a = ntohl(addr->s_addr);
    const char *orig_dst = dst;

    if (size < INET_ADDRSTRLEN) 
	{
      errno = ENOSPC;
      return NULL;
    }
	
    for (i = 0; i < 4; ++i) 
	{
	  int n = (a >> (24 - i * 8)) & 0xFF;
	  int non_zerop = 0;

	  if (non_zerop || n / 100 > 0) 
	  {
	    *dst++ = digits[n / 100];
	    n %= 100;
	    non_zerop = 1;
	  }
	  if (non_zerop || n / 10 > 0) 
	  {
	    *dst++ = digits[n / 10];
	    n %= 10;
	    non_zerop = 1;
	  }
	  *dst++ = digits[n];
	  if (i != 3)
	    *dst++ = '.';
    }
    *dst++ = '\0';
    return orig_dst;
}


