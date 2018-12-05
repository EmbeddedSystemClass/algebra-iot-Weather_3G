#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "FreeRTOS.h"

#include "ssLogging.h"
#include "ssUart.h"

#include "ATCmdParser.h"


#ifdef LF
#undef LF
#define LF  10
#else
#define LF  10
#endif

#ifdef CR
#undef CR
#define CR  13
#else
#define CR  13
#endif

ATCmdParser *atparser_create(int fd)
{
  ATCmdParser *parser = NULL;
  
  parser = pvPortMalloc(sizeof(ATCmdParser));
  
  if(parser)
  {
    parser->_buffer_size = 256;
    parser->_buffer = pvPortMalloc(parser->_buffer_size);
    assert(parser->_buffer);
    atparser_set_timeout(parser, 5000);
    atparser_set_delimiter(parser, "\r");
    atparser_debug_on(parser, true);
    parser->_oobs = NULL;
    parser->_fd = fd;
  }
  
  return parser;
}

void atparser_destroy(ATCmdParser *self)
{
  vPortFree(self->_buffer);
  vPortFree(self);
}

/**
* Sets string of characters to use as line delimiters
*
* @param output_delimiter string of characters to use as line delimiters
*/
void atparser_set_delimiter(ATCmdParser *self, const char *output_delimiter)
{
  self->_output_delimiter = output_delimiter;
  self->_output_delim_size = strlen(output_delimiter);
}

/**
* Allows timeout to be changed between commands
*
* @param timeout timeout of the connection
*/
void atparser_set_timeout(ATCmdParser *self, int timeout)
{
  self->_timeout = timeout;
}

/**
* Allows traces from modem to be turned on or off
*
* @param on set as 1 to turn on traces and vice versa.
*/
void atparser_debug_on(ATCmdParser *self, uint8_t on)
{
  self->_dbg_on = (on) ? 1 : 0;
}


// getc/putc handling with timeouts
int atparser_putc(ATCmdParser *self, char c)
{
  return ssUartWrite(self->_fd, (uint8_t *)&c, 1);
}

int atparser_getc(ATCmdParser *self)
{
  uint8_t c;
  if(ssUartRead(self->_fd, &c, 1, self->_timeout) == 1)
  {
    return (int)c;
  }
  else
  {
    return -1;
  }
}

void atparser_flush(ATCmdParser *self)
{
  
}


// read/write handling with timeouts
int atparser_write(ATCmdParser *self, const char *data, int size)
{
  return ssUartWrite(self->_fd, (const uint8_t *)data, size);
}

int atparser_read(ATCmdParser *self, char *data, int size)
{
  return ssUartRead(self->_fd, (uint8_t *)data, size, self->_timeout);
}


// printf/scanf handling
int atparser_vprintf(ATCmdParser *self, const char *format, va_list args)
{
  
  if (vsprintf(self->_buffer, format, args) < 0) {
    return false;
  }
  
  int i = 0;
  for ( ; self->_buffer[i]; i++) {
    if (atparser_putc(self, self->_buffer[i]) < 0) {
      return -1;
    }
  }
  return i;
}

int atparser_vscanf(ATCmdParser *self, const char *format, va_list args)
{
  // Since format is const, we need to copy it into our buffer to
  // add the line's null terminator and clobber value-matches with asterisks.
  //
  // We just use the beginning of the buffer to avoid unnecessary allocations.
  int i = 0;
  int offset = 0;
  
  while (format[i]) {
    if (format[i] == '%' && format[i+1] != '%' && format[i+1] != '*') {
      self->_buffer[offset++] = '%';
      self->_buffer[offset++] = '*';
      i++;
    } else {
      self->_buffer[offset++] = format[i++];
    }
  }
  
  // Scanf has very poor support for catching errors
  // fortunately, we can abuse the %n specifier to determine
  // if the entire string was matched.
  self->_buffer[offset++] = '%';
  self->_buffer[offset++] = 'n';
  self->_buffer[offset++] = 0;
  
  // To workaround scanf's lack of error reporting, we actually
  // make two passes. One checks the validity with the modified
  // format string that only stores the matched characters (%n).
  // The other reads in the actual matched values.
  //
  // We keep trying the match until we succeed or some other error
  // derails us.
  int j = 0;
  
  while (true) {
    // Ran out of space
    if (j+1 >= self->_buffer_size - offset) {
      return false;
    }
    // Recieve next character
    int c = atparser_getc(self);
    if (c < 0) {
      return -1;
    }
    self->_buffer[offset + j++] = c;
    self->_buffer[offset + j] = 0;
    
    // Check for match
    int count = -1;
    sscanf(self->_buffer+offset, self->_buffer, &count);
    
    // We only succeed if all characters in the response are matched
    if (count == j) {
      // Store the found results
      vsscanf(self->_buffer+offset, format, args);
      return j;
    }
  }
}


// Command parsing with line handling
bool atparser_vsend(ATCmdParser *self, const char *command, va_list args)
{
  // Create and send command
  if (vsprintf(self->_buffer, command, args) < 0) {
    return false;
  }
  
  for (int i = 0; self->_buffer[i]; i++) {
    if (atparser_putc(self, self->_buffer[i]) < 0) {
      return false;
    }
  }
  
  // Finish with newline
  for (size_t i = 0; self->_output_delimiter[i]; i++) {
    if (atparser_putc(self, self->_output_delimiter[i]) < 0) {
      return false;
    }
  }
  
  //ssLoggingPrint(ESsLoggingLevel_Debug, 0, "AT> %s\n", self->_buffer);
  ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, (const char *)self->_buffer, strlen(self->_buffer), "[AT send] ");
  return true;
}

bool atparser_vrecv(ATCmdParser *self, const char *response, va_list args)
{
restart:
  self->_aborted = false;
  // Iterate through each line in the expected response
  while (response[0]) {
    // Since response is const, we need to copy it into our buffer to
    // add the line's null terminator and clobber value-matches with asterisks.
    //
    // We just use the beginning of the buffer to avoid unnecessary allocations.
    int i = 0;
    int offset = 0;
    bool whole_line_wanted = false;
    
    while (response[i]) {
      if (response[i] == '%' && response[i+1] != '%' && response[i+1] != '*') {
        self->_buffer[offset++] = '%';
        self->_buffer[offset++] = '*';
        i++;
      } else {
        self->_buffer[offset++] = response[i++];
        // Find linebreaks, taking care not to be fooled if they're in a %[^\n] conversion specification
        if (response[i - 1] == '\n' && !(i >= 3 && response[i-3] == '[' && response[i-2] == '^')) {
          whole_line_wanted = true;
          break;
        }
      }
    }
    
    // Scanf has very poor support for catching errors
    // fortunately, we can abuse the %n specifier to determine
    // if the entire string was matched.
    self->_buffer[offset++] = '%';
    self->_buffer[offset++] = 'n';
    self->_buffer[offset++] = 0;
    
    //ssLoggingPrint(ESsLoggingLevel_Debug, 0, "AT? %s\n", self->_buffer);
    // To workaround scanf's lack of error reporting, we actually
    // make two passes. One checks the validity with the modified
    // format string that only stores the matched characters (%n).
    // The other reads in the actual matched values.
    //
    // We keep trying the match until we succeed or some other error
    // derails us.
    int j = 0;
    
    while (true) {
      // Receive next character
      int c = atparser_getc(self);
      if (c < 0) {
        //ssLoggingPrint(ESsLoggingLevel_Debug, 0, "AT(Timeout)\n");
        return false;
      }
      // Simplify newlines (borrowed from retarget.cpp)
      if ((c == CR && self->_in_prev != LF) ||
          (c == LF && self->_in_prev != CR)) {
            self->_in_prev = c;
            c = '\n';
          } else if ((c == CR && self->_in_prev == LF) ||
                     (c == LF && self->_in_prev == CR)) {
                       self->_in_prev = c;
                       // onto next character
                       continue;
                     } else {
                       self->_in_prev = c;
                     }
      self->_buffer[offset + j++] = c;
      self->_buffer[offset + j] = 0;
      
      // Check for oob data
      for (struct oob *oob = self->_oobs; oob; oob = oob->next) {
        if ((unsigned)j == oob->len && memcmp(
                                              oob->prefix, self->_buffer+offset, oob->len) == 0) {
                                                ssLoggingPrint(ESsLoggingLevel_Debug, 0, "AT! %s\n", oob->prefix);
                                                oob->cb(oob->param);
                                                
                                                if (self->_aborted) {
                                                  ssLoggingPrint(ESsLoggingLevel_Debug, 0, "AT(Aborted)\n");
                                                  return false;
                                                }
                                                // oob may have corrupted non-reentrant buffer,
                                                // so we need to set it up again
                                                goto restart;
                                              }
      }
      
      // Check for match
      int count = -1;
      if (whole_line_wanted && c != '\n') {
        // Don't attempt scanning until we get delimiter if they included it in format
        // This allows recv("Foo: %s\n") to work, and not match with just the first character of a string
        // (scanf does not itself match whitespace in its format string, so \n is not significant to it)
      } else {
        sscanf(self->_buffer+offset, self->_buffer, &count);
      }
      
      // We only succeed if all characters in the response are matched
      if (count == j) {
        //ssLoggingPrint(ESsLoggingLevel_Debug, 0, "AT= %s\n", self->_buffer+offset);
        ssLoggingPrintRawStr(ESsLoggingLevel_Debug, 0, self->_buffer+offset, strlen(self->_buffer+offset), "[AT recv] ");
        // Reuse the front end of the buffer
        memcpy(self->_buffer, response, i);
        self->_buffer[i] = 0;
        
        // Store the found results
        vsscanf(self->_buffer+offset, self->_buffer, args);
        
        // Jump to next line and continue parsing
        response += i;
        break;
      }
      
      // Clear the buffer when we hit a newline or ran out of space
      // running out of space usually means we ran into binary data
      if (c == '\n' || j+1 >= self->_buffer_size - offset) {
        //ssLoggingPrint(ESsLoggingLevel_Debug, 0, "AT< %s", self->_buffer+offset);
        j = 0;
      }
    }
  }
  
  return true;
}

// Mapping to vararg functions
int atparser_printf(ATCmdParser *self, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  int res = vprintf(format, args);
  va_end(args);
  return res;
}

int atparser_scanf(ATCmdParser *self, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  int res = vscanf(format, args);
  va_end(args);
  return res;
}

bool atparser_send(ATCmdParser *self, const char *command, ...)
{
  va_list args;
  va_start(args, command);
  bool res = atparser_vsend(self, command, args);
  va_end(args);
  return res;
}

bool atparser_recv(ATCmdParser *self, const char *response, ...)
{
  va_list args;
  va_start(args, response);
  bool res = atparser_vrecv(self, response, args);
  va_end(args);
  return res;
}

// oob registration
void atparser_oob(ATCmdParser *self, const char *prefix, void (*cb)(void *), void *param)
{
  struct oob *oob = malloc(sizeof(struct oob));
  oob->len = strlen(prefix);
  oob->prefix = prefix;
  oob->cb = cb;
  oob->param = param;
  oob->next = self->_oobs;
  self->_oobs = oob;
}

void atparser_abort(ATCmdParser *self)
{
  self->_aborted = true;
}

bool atparser_process_oob(ATCmdParser *self)
{
  //if (!_fh->readable()) {
  //    return false;
  //}
  
  int i = 0;
  while (true) {
    // Receive next character
    int c = atparser_getc(self);
    if (c < 0) {
      return false;
    }
    self->_buffer[i++] = c;
    self->_buffer[i] = 0;
    
    // Check for oob data
    struct oob *oob = self->_oobs;
    while (oob) {
      if (i == (int)oob->len && memcmp(
                                       oob->prefix, self->_buffer, oob->len) == 0) {
                                         ssLoggingPrint(ESsLoggingLevel_Debug, 0, "AT! %s\r\n", oob->prefix);
                                         oob->cb(oob->param);
                                         return true;
                                       }
      oob = oob->next;
    }
    
    // Clear the buffer when we hit a newline or ran out of space
    // running out of space usually means we ran into binary data
    if (i+1 >= self->_buffer_size ||
        strcmp(&self->_buffer[i-self->_output_delim_size], self->_output_delimiter) == 0) {
          
          ssLoggingPrint(ESsLoggingLevel_Debug, 0, "AT< %s", self->_buffer);
          i = 0;
        }
  }
}
