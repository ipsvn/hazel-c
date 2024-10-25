#pragma once

#include "hazel/common.h"
#include "hazel/udp/socket.h"
#include "hazel/udp/connection.h"
#include "hazel/ip_mode.h"

/** \addtogroup UDP_Client UDP Client
 *  \ingroup UDP
 *  \brief A UDP client for connecting to a hazel connection listener/server
 *  @{
 */

typedef struct hazel_udp_client
{
    hazel_udp_connection udp_connection;
} hazel_udp_client;

#define HAZEL_UDP_CLIENT_RECV_NO_ERROR 0x00
#define HAZEL_UDP_CLIENT_RECV_NO_MESSAGE -0xC100
#define HAZEL_UDP_CLIENT_RECV_HAS_MESSAGE 0x01
#define HAZEL_UDP_CLIENT_RECV_HAS_DISCONNECTED 0x02

#define HAZEL_UDP_CLIENT_HANDSHAKE_SUCCESS 0x00
#define HAZEL_UDP_CLIENT_HANDSHAKE_FAILURE -0xC200
#define HAZEL_UDP_CLIENT_HANDSHAKE_TIMEOUT -0xC201
#define HAZEL_UDP_CLIENT_HANDSHAKE_DISCONNECTED -0xC202

int hazel_udp_client_init(hazel_udp_client* client, const char* hostname, 
                           uint16_t port, enum hazel_ip_mode ip_mode);
void hazel_udp_client_free(hazel_udp_client* client);

int hazel_udp_client_close(hazel_udp_client* client);

/**
 * Completes the client handshake to a hazel server.
 * 
 * \param client     The client structure
 * \param buffer     A byte buffer to append to the hello message.
 *                   Pass NULL if no additional data is desired.
 * \param buffer_len The length of buffer \p buffer
 * \return #HAZEL_UDP_CLIENT_HANDSHAKE_SUCCESS if successful
 * \return #HAZEL_UDP_CLIENT_HANDSHAKE_FAILURE for unknown error
 * \return #HAZEL_UDP_CLIENT_HANDSHAKE_TIMEOUT if timed out
 * \return \c 0 if successful
*/
int hazel_udp_client_handshake(hazel_udp_client* client, 
                               uint8_t* buffer, size_t buffer_len);

int hazel_udp_client_recv(hazel_udp_client* client, 
                          enum hazel_send_option* out_send_option, 
                          hazel_message_reader* out_reader);

/** @}*/
