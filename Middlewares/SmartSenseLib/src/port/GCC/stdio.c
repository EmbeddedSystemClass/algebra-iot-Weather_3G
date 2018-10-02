/**
* @file     
* @brief    
* @warning
* @details
*
* Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
*
**/

#include <errno.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/unistd.h>
#include "bsp.h"


#ifndef STDOUT_USART
#define STDOUT_USART UART4
#endif

#ifndef STDERR_USART
#define STDERR_USART UART4
#endif

#ifndef STDIN_USART
#define STDIN_USART UART4
#endif

#undef errno
extern int errno;

extern int32_t stdin_uart_handle;
extern int32_t stdout_uart_handle;
extern int32_t stderr_uart_handle;

/*
 read
 Read a character to a file. `libc' subroutines will use this system routine for input from all files, including stdin
 Returns -1 on error or blocks until the number of characters have been read.
 */


int _read(int file, char *ptr, int len) {
    
    return 0;
}


/*
 write
 Write a character to a file. `libc' subroutines will use this system routine for output to all files, including stdout
 Returns -1 on error or number of bytes sent
 */
int _write(int fd, const unsigned char * buffer, size_t size)
{
  int32_t huart;
  
  if(fd == STDOUT_FILENO)
  {
    huart = stdout_uart_handle;
  }
  else if(fd == STDERR_FILENO)
  {
    huart = stderr_uart_handle;
  }
  else
  {
    // error
    return 0;
  }
  
  return (size_t)ssUartWrite(huart, buffer, size);
}

