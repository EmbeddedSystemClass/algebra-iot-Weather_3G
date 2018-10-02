
#ifndef _ATCMDPARSER_H
#define _ATCMDPARSER_H

#include <stdarg.h>
#include <stdbool.h>
/** \addtogroup platform */
/** @{*/
/**
* \defgroup platform_ATCmdParser ATCmdParser class
* @{
*/

/**
* Parser class for parsing AT commands
*
* Here are some examples:
* @code
* UARTSerial serial = UARTSerial(D1, D0);
* ATCmdParser at = ATCmdParser(&serial, "\r\n");
* int value;
* char buffer[100];
*
* at.send("AT") && at.recv("OK");
* at.send("AT+CWMODE=%d", 3) && at.recv("OK");
* at.send("AT+CWMODE?") && at.recv("+CWMODE:%d\r\nOK", &value);
* at.recv("+IPD,%d:", &value);
* at.read(buffer, value);
* at.recv("OK");
* @endcode
*/

typedef struct oob {
  unsigned len;
  const char *prefix;
  void (*cb)(void *);
  void *param;
  struct oob *next;
} oob;

typedef struct ATCmdParser
{
  // File handle
  // Not owned by ATCmdParser
  int _fd;
  
  int _buffer_size;
  char *_buffer;
  int _timeout;
  
  // Parsing information
  const char *_output_delimiter;
  int _output_delim_size;
  char _in_prev;
  bool _dbg_on;
  bool _aborted;
  oob *_oobs;
} ATCmdParser;


ATCmdParser *atparser_create(int fd);
void atparser_destroy(ATCmdParser *self);


/**
* Allows timeout to be changed between commands
*
* @param timeout timeout of the connection
*/
void atparser_set_timeout(ATCmdParser *self, int timeout);


/**
* Sets string of characters to use as line delimiters
*
* @param output_delimiter string of characters to use as line delimiters
*/
void atparser_set_delimiter(ATCmdParser *self, const char *output_delimiter);



/**
* Allows traces from modem to be turned on or off
*
* @param on set as 1 to turn on traces and vice versa.
*/
void atparser_debug_on(ATCmdParser *self, uint8_t on);


/**
* Sends an AT command
*
* Sends a formatted command using printf style formatting
* @see printf
*
* @param command printf-like format string of command to send which
*                is appended with a newline
* @param ... all printf-like arguments to insert into command
* @return true only if command is successfully sent
*/
bool atparser_send(ATCmdParser *self, const char *command, ...);

bool atparser_vsend(ATCmdParser *self, const char *command, va_list args);

/**
* Receive an AT response
*
* Receives a formatted response using scanf style formatting
* @see scanf
*
* Responses are parsed line at a time.
* Any received data that does not match the response is ignored until
* a timeout occurs.
*
* @param response scanf-like format string of response to expect
* @param ... all scanf-like arguments to extract from response
* @return true only if response is successfully matched
*/
bool atparser_recv(ATCmdParser *self, const char *response, ...);

bool atparser_vrecv(ATCmdParser *self, const char *response, va_list args);

/**
* Write a single byte to the underlying stream
*
* @param c The byte to write
* @return The byte that was written or -1 during a timeout
*/
int atparser_putc(ATCmdParser *self, char c);

/**
* Get a single byte from the underlying stream
*
* @return The byte that was read or -1 during a timeout
*/
int atparser_getc(ATCmdParser *self);

/**
* Write an array of bytes to the underlying stream
*
* @param data the array of bytes to write
* @param size number of bytes to write
* @return number of bytes written or -1 on failure
*/
int atparser_write(ATCmdParser *self, const char *data, int size);

/**
* Read an array of bytes from the underlying stream
*
* @param data the destination for the read bytes
* @param size number of bytes to read
* @return number of bytes read or -1 on failure
*/
int atparser_read(ATCmdParser *self, char *data, int size);

/**
* Direct printf to underlying stream
* @see printf
*
* @param format format string to pass to printf
* @param ... arguments to printf
* @return number of bytes written or -1 on failure
*/
int atparser_printf(ATCmdParser *self, const char *format, ...);

int atparser_vprintf(ATCmdParser *self, const char *format, va_list args);

/**
* Direct scanf on underlying stream
* @see scanf
*
* @param format format string to pass to scanf
* @param ... arguments to scanf
* @return number of bytes read or -1 on failure
*/
int atparser_scanf(ATCmdParser *self, const char *format, ...);

int atparser_vscanf(ATCmdParser *self, const char *format, va_list args);

/**
* Attach a callback for out-of-band data
*
* @param prefix string on when to initiate callback
* @param func callback to call when string is read
* @note out-of-band data is only processed during a scanf call
*/
void atparser_oob(ATCmdParser *self, const char *prefix, void (*cb)(void *), void *param);

/**
* Flushes the underlying stream
*/
void atparser_flush(ATCmdParser *self);

/**
* Abort current recv
*
* Can be called from oob handler to interrupt the current
* recv operation.
*/
void atparser_abort(ATCmdParser *self);

/**
* Process out-of-band data
*
* Process out-of-band data in the receive buffer. This function
* returns immediately if there is no data to process.
*
* @return true if oob data processed, false otherwise
*/
bool atparser_process_oob(ATCmdParser *self);

/**@}*/

/**@}*/

#endif //ATCMDPARSER_H
