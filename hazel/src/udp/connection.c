#include "hazel/udp/connection.h"
#include "hazel/errors.h"

#include "../utils.h"

#include <stdlib.h>

int hazel_udp_connection_init(hazel_udp_connection *connection)
{
    connection->_connection_state = HAZEL_CONNECTION_STATE_NOT_CONNECTED;
    connection->last_reliable_id = -1;
    connection->reliable_packets = NULL;
    hazel_udp_socket_init(&connection->_socket);
    return 0;
}

void hazel_udp_connection_free(hazel_udp_connection *connection)
{
    hazel_udp_socket_free(&connection->_socket);
}

int hazel_udp_connection_close(hazel_udp_connection *connection)
{
    int ret = 0;
    ret |= hazel_udp_socket_close(&connection->_socket);
    return ret;
}

int hazel_udp_connection_send(hazel_udp_connection *connection,
                              hazel_message_writer *writer)
{
    if (connection->_connection_state != HAZEL_CONNECTION_STATE_CONNECTED)
    {
        return HAZEL_UDP_CONNECTION_NOT_CONNECTED;
    }

    uint8_t buffer[1024];
    size_t out_size;
    hazel_message_writer_to_bytes(writer, buffer, sizeof(buffer), 
                                  true, &out_size);

    if (writer->send_option == HAZEL_SEND_OPTION_RELIABLE)
    {
        hazel_udp_connection_make_reliable(
                connection, buffer, sizeof(buffer), 1, NULL);
    }

    HAZEL_LOG_DEBUG_PRINT_BYTES("send to socket", buffer, out_size, 0);

    hazel_udp_socket_send(&connection->_socket, buffer, out_size, 0);

    return 0;
}

int hazel_udp_connection_send_ack(hazel_udp_connection *connection,
                                  uint16_t reliable_id)
{
    // TODO reliable packets

    uint8_t arr[4] = { HAZEL_SEND_OPTION_ACK, (uint8_t)(reliable_id >> 8), (uint8_t)reliable_id, 0xFF };

    int ret;
    if ((ret = hazel_udp_socket_send(&connection->_socket, arr, 4, 0)) < 0)
    {
        return ret;
    }

    return 0;
}

int hazel_udp_connection_make_reliable(hazel_udp_connection *connection,
                                       uint8_t *buffer, size_t buffer_size,
                                       size_t offset, uint16_t *out_id)
{
    if (offset >= buffer_size)
    {
        return HAZEL_ERR_UNKNOWN;
    }

    uint16_t id = ++connection->last_reliable_id;
    
    buffer[offset] = (id >> 8);
    buffer[offset + 1] = id;

    if (out_id != NULL)
    {
        *out_id = id;
    }

    return 0;
    // TODO retransmission
}

int hazel_udp_connection_malloc_reader(
    uint8_t *buffer, size_t buffer_size,
    size_t offset, hazel_message_reader *reader)
{
    uint8_t *reader_buffer = malloc(buffer_size - offset);
    if (reader_buffer == NULL)
    {
        return HAZEL_ERR_FAILED_ALLOC;
    }
    memcpy(reader_buffer, buffer + offset, buffer_size - offset);

    int ret = hazel_message_reader_init(
        reader, reader_buffer,
        buffer_size - offset, 0);
    if (ret < 0)
    {
        return ret;
    }
    return 0;
}

int hazel_udp_connection_handle_message(
    hazel_udp_connection* connection, uint8_t *buffer, size_t buffer_size,
    hazel_udp_connection_recv *out_recv_data, bool reliable)
{
    int ret;

    enum hazel_send_option send_option = reliable 
        ? HAZEL_SEND_OPTION_RELIABLE 
        : HAZEL_SEND_OPTION_UNRELIABLE;
    out_recv_data->packet_type = send_option;

    size_t offset = 1;

    if (reliable)
    {
        uint16_t reliable_id = (buffer[1] << 8) + buffer[2];
        if ((ret = hazel_udp_connection_send_ack(connection, reliable_id)) < 0)
        {
            return ret;
        }
        offset = 3;
    }

    if ((ret = hazel_udp_connection_malloc_reader(
        buffer, buffer_size, 
        offset, &out_recv_data->data.msg.reader)) < 0)
    {
        return ret;
    }
    
    return 0;
}

int hazel_udp_connection_handle_disconnect(
    hazel_udp_connection* connection, uint8_t *buffer, size_t buffer_size,
    hazel_udp_connection_recv *out_recv_data)
{
    int ret;

    out_recv_data->packet_type = HAZEL_SEND_OPTION_DISCONNECT;

    // TODO maybe handle this somewhere else
    connection->_connection_state = HAZEL_CONNECTION_STATE_NOT_CONNECTED;

    size_t offset = 1;

    if ((ret = hazel_udp_connection_malloc_reader(
        buffer, buffer_size, 
        offset, &out_recv_data->data.disconnect.reader)) < 0)
    {
        return ret;
    }
    
    return 0;
}


int hazel_udp_connection_handle_recv(hazel_udp_connection *connection,
                                     uint8_t *buffer, size_t buffer_size,
                                     hazel_udp_connection_recv *out_recv_data)
{
    if (buffer_size < 1)
    {
        return HAZEL_ERR_UNKNOWN;
    }

    HAZEL_LOG_DEBUG_PRINT_BYTES("hazel_udp_connection_handle_recv", buffer, 
                                buffer_size, 0);

    enum hazel_send_option send_option = buffer[0];

    switch(send_option)
    {
        case HAZEL_SEND_OPTION_UNRELIABLE:
        case HAZEL_SEND_OPTION_RELIABLE:
            return hazel_udp_connection_handle_message(
                connection, buffer, buffer_size, out_recv_data,
                send_option == HAZEL_SEND_OPTION_RELIABLE);
        case HAZEL_SEND_OPTION_DISCONNECT:
            return hazel_udp_connection_handle_disconnect(
                connection, buffer, buffer_size, out_recv_data);
        case HAZEL_SEND_OPTION_ACK:
            if (buffer_size != 4)
            {
                return HAZEL_ERR_UNKNOWN;
            }
            out_recv_data->packet_type = HAZEL_SEND_OPTION_ACK;
            out_recv_data->data.ack.reliable_id = (buffer[1] << 8) + buffer[2];
            out_recv_data->data.ack.recent_packets = buffer[3];
            break;
        case HAZEL_SEND_OPTION_PING:
            if (buffer_size != 3)
            {
                return HAZEL_ERR_UNKNOWN;
            }
            out_recv_data->packet_type = HAZEL_SEND_OPTION_PING;
            out_recv_data->data.ping.reliable_id = (buffer[1] << 8) + buffer[2];
            // TODO reliable packets
            hazel_udp_connection_send_ack(connection, 
                                          out_recv_data->data.ping.reliable_id);
            break;
        default:
            HAZEL_LOG_ERROR("Received unknown packet of type %d", buffer[0]);
            break;
    }

    return 0;
}
int hazel_udp_connection_manage_reliable(hazel_udp_connection *connection)
{
    HAZEL_UNUSED(connection); // TODO
    HAZEL_LOG_DEBUG("manage reliable");
    return 0;
}
