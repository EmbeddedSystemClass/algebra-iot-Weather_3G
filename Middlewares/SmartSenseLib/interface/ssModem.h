/**
* @file
* @brief
* @warning
* @details
*
* Copyright (c) Smart Sense d.o.o 2016. All rights reserved.
*
**/

#ifndef _SS_MODEM_H
#define _SS_MODEM_H

#ifdef __cplusplus
extern "C" {
#endif

/*------------------------- MACRO DEFINITIONS --------------------------------*/
//#define LOCK()    {
//#define UNLOCK()  }
  
  
#define SOCKET_COUNT    7
#define SOCKET_BUF_SIZE 1024
  
#define SOCKET_INVALID (-1)
  
#define SOCKET_CLOSED   0
#define SOCKET_OPENED   1
  
#define SOCK_IP_SIZE  40
#define SOCK_IP_BYTES 16

#define SS_AF_INET    2

//#define SOCK_STREAM                         (1)                       /*TCP Socket                                                          */
//#define SOCK_DGRAM                          (2)                       /*UDP Socket                                                          */
//#define SOCK_RAW                            (3)                       /*Raw socket                                                          */
#define IPPROTO_RAW                         (255)                     /*Raw Socket                                                          */
#define SEC_SOCKET                          (100)                     /* Secured Socket Layer (SSL,TLS)                                      */
#define FNET_SA_DATA_SIZE 100
  
  
/*------------------------- TYPE DEFINITIONS ---------------------------------*/

typedef enum {
  CSD_NOT_REGISTERED_NOT_SEARCHING = 0,
  CSD_REGISTERED = 1,
  CSD_NOT_REGISTERED_SEARCHING = 2,
  CSD_REGISTRATION_DENIED = 3,
  CSD_UNKNOWN_COVERAGE = 4,
  CSD_REGISTERED_ROAMING = 5,
  CSD_SMS_ONLY = 6,
  CSD_SMS_ONLY_ROAMING = 7,
  CSD_CSFB_NOT_PREFERRED = 9
} NetworkRegistrationStatusCsd;

/** Packet Switched network registration status (CGREG Usage).
* UBX-13001820 - AT Commands Example Application Note (Section 18.27.3).
*/
typedef enum {
  PSD_NOT_REGISTERED_NOT_SEARCHING = 0,
  PSD_REGISTERED = 1,
  PSD_NOT_REGISTERED_SEARCHING = 2,
  PSD_REGISTRATION_DENIED = 3,
  PSD_UNKNOWN_COVERAGE = 4,
  PSD_REGISTERED_ROAMING = 5,
  PSD_EMERGENCY_SERVICES_ONLY = 8
} NetworkRegistrationStatusPsd;

/** EPS network registration status (CEREG Usage).
* UBX-13001820 - AT Commands Example Application Note (Section 18.36.3).
*/
typedef enum {
  EPS_NOT_REGISTERED_NOT_SEARCHING = 0,
  EPS_REGISTERED = 1,
  EPS_NOT_REGISTERED_SEARCHING = 2,
  EPS_REGISTRATION_DENIED = 3,
  EPS_UNKNOWN_COVERAGE = 4,
  EPS_REGISTERED_ROAMING = 5,
  EPS_EMERGENCY_SERVICES_ONLY = 8
} NetworkRegistrationStatusEps;

/** Supported u-blox modem variants.
*/
typedef enum {
  DEV_TYPE_NONE = 0,
  DEV_SARA_N2,
  DEV_SARA_U2,
} DeviceType;

/** Network registration status.
* UBX-13001820 - AT Commands Example Application Note (Section 4.1.4.5).
*/
typedef enum {
  GSM = 0,
  COMPACT_GSM = 1,
  UTRAN = 2,
  EDGE = 3,
  HSDPA = 4,
  HSUPA = 5,
  HSDPA_HSUPA = 6,
  LTE = 7,
  EC_GSM_IoT = 8,
  E_UTRAN_NB_S1 = 9,
  RAN_TYPE_LAST
} RadioAccessNetworkType;

/** Info about the modem.
*/
typedef struct {
  DeviceType dev;
  char iccid[20 + 1];   //!< Integrated Circuit Card ID.
  char imsi[15 + 1];    //!< International Mobile Station Identity.
  char imei[15 + 1];    //!< International Mobile Equipment Identity.
  char meid[18 + 1];    //!< Mobile Equipment IDentifier.
  volatile RadioAccessNetworkType rat;  //!< Type of network (e.g. 2G, 3G, LTE).
  volatile NetworkRegistrationStatusCsd reg_status_csd; //!< Circuit switched attach status.
  volatile NetworkRegistrationStatusPsd reg_status_psd; //!< Packet switched attach status.
  volatile NetworkRegistrationStatusEps reg_status_eps; //!< Evolved Packet Switched (e.g. LTE) attach status.
} DeviceInfo;
  
typedef struct SockCtrl
{
  uint32_t state;
  volatile uint32_t pending;
  uint8_t *buffer;
} SockCtrl;

typedef struct SocketAddress
{
    uint8_t   sin_family;     
    char      sin_addr[14];       
} SocketAddress;

typedef struct SocketAddress_in
{
    uint8_t   sin_family;     
    uint16_t  sin_port;
    char      sin_addr[INET_ADDRSTRLEN];       
} SocketAddress_in;
  
  
typedef struct modem_t
{
  netif_t com_dev;
  char *ipaddr;
  const char *apn;
  const char *uname;
  const char *pwd;
  bool sim_pin_enabled;
  const char *pin;
  ATCmdParser *at;
  int at_timeout;
  int fd;
  DeviceInfo dev_info;
  bool modem_initialised;
  SockCtrl sockets[SOCKET_COUNT];
} modem_t;
    
  /*------------------------- PUBLIC VARIABLES ---------------------------------*/
  /*------------------------- PUBLIC FUNCTION PROTOTYPES -----------------------*/
  
  modem_t *modem_create(void);
  void modem_destroy(modem_t *self);
  
  bool modem_init(modem_t *self, const char *pin);
  
  bool modem_nwk_register(modem_t *self);
  bool modem_nwk_deregister(modem_t *self);
  
  bool modem_nwk_connect(modem_t *self);
  bool modem_nwk_disconnect(modem_t *self);
  
  void  modem_set_credentials(modem_t *self,
                              const char *apn,
                              const char *uname,
                              const char *pwd);
  
  int32_t modem_gethostbyname(modem_t *self,
                              const char *host,
                              uint32_t *address);
  
  int modem_socket_open(modem_t *self, int protocol);
  bool modem_socket_close(modem_t *self, int socket);
  int16_t modem_socket_connect(modem_t *self, int socket,
                           const struct SocketAddress_in *address);

  int16_t modem_socket_send(modem_t *self,
                            int socket,
                            const void *message,
                            size_t length);

  int16_t modem_socket_sendto(modem_t *self,
                              int socket,
                              const void *message,
                              size_t length,
                              const struct SocketAddress_in *dest_addr);
  int16_t modem_socket_recv(modem_t *self,
                                int socket,
                                void *buffer,
                                size_t length);
  int16_t modem_socket_recvfrom(modem_t *self,
                                int socket,
                                void *buffer,
                                size_t length,
                                struct SocketAddress_in *restrict address);
  /*------------------------- PUBLIC FUNCTION DEFINITIONS ----------------------*/
  
#ifdef __cplusplus
}
#endif

#endif /* _SS_MODEM_H */
