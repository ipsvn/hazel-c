#pragma once

#include "hazel/common.h"
#include "hazel/ip_mode.h"

#include <string.h>
#include <stdint.h>
#include <stddef.h>

#include <sys/types.h>

/** \defgroup UDP UDP
 **/

/** \addtogroup UDP_Socket UDP Socket
 *  \ingroup UDP
 *  \brief Base implementation of UDP socket  
 *  @{
 */

#include <time.h>

#define HAZEL_UDP_SOCKET_SOCKET_ERROR -0xA001

#define HAZEL_UDP_SOCKET_CONN_REFUSED -0xA101

#define HAZEL_UDP_SOCKET_RECV_NO_MESSAGE -0xA201
#define HAZEL_UDP_SOCKET_RECV_ERROR -0xA202

#define HAZEL_UDP_SOCKET_SEND_ERROR -0xA302

typedef struct hazel_udp_socket
{
    enum hazel_ip_mode ip_mode;

    int _sock_handle;
} hazel_udp_socket;


int hazel_udp_socket_init(hazel_udp_socket* socket);
void hazel_udp_socket_free(hazel_udp_socket* socket);

int hazel_udp_socket_open(hazel_udp_socket* socket, enum hazel_ip_mode ip_mode);
int hazel_udp_socket_close(hazel_udp_socket* socket);

int hazel_udp_socket_connect(hazel_udp_socket* socket, 
                          const char* hostname, int port);

int hazel_udp_socket_recv(hazel_udp_socket* socket, uint8_t* buffer, 
                              size_t size, int flags, int timeout);
int hazel_udp_socket_send(hazel_udp_socket* socket, uint8_t* buffer, 
                              size_t size, int flags);

/** @}*/
