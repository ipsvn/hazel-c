#pragma once

#include "hazel/common.h"
#include "hazel/connection_state.h"
#include "hazel/errors.h"
#include "hazel/reader.h"
#include "hazel/writer.h"
#include "hazel/send_option.h"
#include "socket.h"

#include <time.h>

/** \addtogroup UDP_Connection UDP Connection
 *  \ingroup UDP
 *  \brief Base hazel UDP connection, for both client and server connections
 *  @{
 */

#define HAZEL_UDP_CONNECTION_HANDLE_RECV_INVALID_MSG -0xB001
#define HAZEL_UDP_CONNECTION_NOT_CONNECTED -0xB002

typedef struct hazel_udp_connection hazel_udp_connection;
typedef struct hazel_udp_sent_packet hazel_udp_sent_packet;

typedef void (*ack_callback)(hazel_udp_connection *connection,
                             hazel_udp_sent_packet *packet);

typedef struct hazel_udp_sent_packet
{
    uint16_t id;
    uint32_t length;
    bool acked;
    uint8_t retransmission_count;
    struct timespec created_at;
    struct timespec last_transmission;

    ack_callback callback_func;

    struct hazel_udp_sent_packet *next_packet;
} hazel_udp_sent_packet;

typedef struct hazel_udp_connection
{
    enum hazel_connection_state _connection_state;
 
    hazel_udp_socket _socket;

    uint16_t last_reliable_id;
    hazel_udp_sent_packet *reliable_packets;

} hazel_udp_connection;

int hazel_udp_connection_init(hazel_udp_connection *connection);
void hazel_udp_connection_free(hazel_udp_connection *connection);

int hazel_udp_connection_close(hazel_udp_connection *connection);

int hazel_udp_connection_send(hazel_udp_connection *connection,
                              hazel_message_writer *writer);

/**
 * Make a packet reliable by adding a 2-byte reliable ID at an offset and adding
 * it to a list for retransmission
 * @param connection The connection structure
 * @param buffer The buffer to write the reliable ID into, and to store for
 * retransmission
 * @param buffer_size The buffer size to store for retransmission
 * @param offset Offset to write the reliable ID into buffer with
 * @param out_id The ID used on the packet, may be NULL
 */
int hazel_udp_connection_make_reliable(hazel_udp_connection *connection,
                                       uint8_t *buffer, size_t buffer_size,
                                       size_t offset, uint16_t *out_id);

typedef struct hazel_udp_connection_recv_ack
{
    uint16_t reliable_id;
    uint8_t recent_packets; // TODO reliable packets
} hazel_udp_connection_recv_ack;

typedef struct hazel_udp_connection_recv_ping
{
    uint16_t reliable_id;
} hazel_udp_connection_recv_ping;

typedef struct hazel_udp_connection_recv_msg
{
    hazel_message_reader reader;
} hazel_udp_connection_recv_msg;

typedef struct hazel_udp_connection_recv_disconnect
{
    hazel_message_reader reader;
} hazel_udp_connection_recv_disconnect;

typedef union {
    hazel_udp_connection_recv_ack ack;
    hazel_udp_connection_recv_ping ping;
    hazel_udp_connection_recv_msg msg;
    hazel_udp_connection_recv_disconnect disconnect;
} hazel_udp_connection_recv_data;

typedef struct hazel_udp_connection_recv
{
    enum hazel_send_option packet_type;

    hazel_udp_connection_recv_data data;

} hazel_udp_connection_recv;

int hazel_udp_connection_handle_recv(hazel_udp_connection *connection,
                                     uint8_t *buffer, size_t buffer_size,
                                     hazel_udp_connection_recv *out_recv_data);

int hazel_udp_connection_manage_reliable(hazel_udp_connection *connection);

int hazel_udp_connection_disconnect(hazel_udp_connection *connection);

/** @}*/
