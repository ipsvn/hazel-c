#include "hazel/udp/client.h"

#include "../utils.h"
#include <malloc.h>
#include <errno.h>

#define HAZEL_UDP_RECV_TIMEOUT_MS 100

int hazel_udp_client_init(hazel_udp_client *client, const char *hostname,
                              uint16_t port, enum hazel_ip_mode ip_mode)
{
    hazel_udp_connection_init(&client->udp_connection);

    int ret;
    if ((ret = hazel_udp_socket_init(&client->udp_connection._socket)) != 0)
    {
        HAZEL_LOG_DEBUG("hazel_udp_socket_init failed: %d", ret);
        return ret;
    }
    if ((ret = hazel_udp_socket_open(&client->udp_connection._socket,
                                     ip_mode)) != 0)
    {
        HAZEL_LOG_DEBUG("hazel_udp_socket_open failed: %d", ret);
        return ret;
    }
    if ((ret = hazel_udp_socket_connect(&client->udp_connection._socket,
                                        hostname, port)) != 0)
    {
        HAZEL_LOG_DEBUG("hazel_udp_socket_connect failed: %d", ret);
        return ret;
    }

    HAZEL_LOG_DEBUG("hazel_udp_client_init successful");
    return 0;
}

void hazel_udp_client_free(hazel_udp_client* client)
{
    hazel_udp_connection_free(&client->udp_connection);
}

int hazel_udp_client_close(hazel_udp_client *client)
{
    hazel_udp_connection_close(&client->udp_connection);
    return 0;
}

int hazel_udp_client_handshake(hazel_udp_client *client,
                               uint8_t *in_buffer, size_t in_buffer_len)
{
    hazel_udp_socket *socket = &client->udp_connection._socket;

    int ret;

    uint8_t hello_prefix[] = {0x08, 0x00, 0x00, 0x00};
    size_t hello_prefix_len = ARRAY_LENGTH(hello_prefix);

    uint8_t *hello_buf = hello_prefix;
    size_t hello_buf_len = hello_prefix_len;

    bool has_buffer = in_buffer != NULL && in_buffer_len > 0;
    if (has_buffer)
    {
        hello_buf_len += in_buffer_len;
        uint8_t *temp = malloc(hello_buf_len);
        memcpy(temp, hello_prefix, hello_prefix_len);
        memcpy(temp + hello_prefix_len, in_buffer, in_buffer_len);
        hello_buf = temp;
    }

    uint16_t hello_reliable_id;
    ret = hazel_udp_connection_make_reliable(&client->udp_connection, hello_buf,
                                             hello_buf_len, 1, 
                                             &hello_reliable_id);
    if (ret < 0)
    {
        HAZEL_LOG_DEBUG("hazel_udp_connection_make_reliable failed: %d", ret);
        goto exit;
    }

    if ((ret = hazel_udp_socket_send(socket, hello_buf, hello_buf_len, 0)) < 0)
    {
        HAZEL_LOG_DEBUG("hazel_udp_socket_send failed: %d", ret);
        ret = HAZEL_UDP_SOCKET_SEND_ERROR;
        goto exit;
    }

    uint8_t buffer[HAZEL_BUFFER_SIZE];
    while (true)
    {
        do
        {
            ret = hazel_udp_socket_recv(socket, buffer, HAZEL_BUFFER_SIZE, 0,
                                        HAZEL_UDP_RECV_TIMEOUT_MS);
        } while (ret == HAZEL_UDP_SOCKET_RECV_NO_MESSAGE);

        if (ret < 0)
        {
            HAZEL_LOG_DEBUG("hazel_udp_socket_recv failed: %d", ret);
            goto exit;
        }

        int size = ret;

        // We need to handle the acknowledgement here, so we are manually
        // listening off the socket and using this.
        hazel_udp_connection_recv recv_data;
        ret = hazel_udp_connection_handle_recv(&client->udp_connection,
                                               buffer, size, &recv_data);

        if (ret < 0)
        {
            HAZEL_LOG_DEBUG("hazel_udp_connection_handle_recv failed: %d", ret);
            goto exit;
        }

        if (recv_data.packet_type == HAZEL_SEND_OPTION_DISCONNECT)
        {
            ret = HAZEL_UDP_CLIENT_HANDSHAKE_DISCONNECTED;
            HAZEL_LOG_DEBUG("hazel_udp_client_handshake received disconnect request");
            goto exit;
        }

        if (recv_data.packet_type != HAZEL_SEND_OPTION_ACK)
        {
            ret = HAZEL_UDP_CLIENT_HANDSHAKE_FAILURE;
            HAZEL_LOG_DEBUG("hazel_udp_client_handshake received non ack packet");
            goto exit;
        }

        if (recv_data.data.ack.reliable_id != hello_reliable_id)
        {
            ret = HAZEL_UDP_CLIENT_HANDSHAKE_FAILURE;
            HAZEL_LOG_DEBUG("hazel_udp_client_handshake: ack containing other reliable_id received");
            goto exit;
        }

        ret = HAZEL_UDP_CLIENT_HANDSHAKE_SUCCESS;
        client->udp_connection._connection_state 
            = HAZEL_CONNECTION_STATE_CONNECTED;
        goto exit;
    }

exit:
    if (has_buffer)
    {
        free(hello_buf);
    }

    return ret;
}

int hazel_udp_client_recv(hazel_udp_client *client, enum hazel_send_option *out_send_option,
                          hazel_message_reader *out_reader)
{
    hazel_udp_socket *socket = &client->udp_connection._socket;
    int ret;

    uint8_t buffer[HAZEL_BUFFER_SIZE];
    ret = hazel_udp_socket_recv(socket, buffer, HAZEL_BUFFER_SIZE,
                                0, HAZEL_UDP_RECV_TIMEOUT_MS);

    HAZEL_LOG_DEBUG("hazel_udp_socket_recv ret: %d", ret);

    if (ret == HAZEL_UDP_SOCKET_RECV_NO_MESSAGE)
    {
        return HAZEL_UDP_CLIENT_RECV_NO_MESSAGE;
    }

    if (ret < 0)
    {
        return ret;
    }

    if (ret > 0)
    {
        int size = ret;
        
        hazel_udp_connection_recv recv_data;
        ret = hazel_udp_connection_handle_recv(&client->udp_connection,
                                               buffer, size, &recv_data);

        if (recv_data.packet_type == HAZEL_SEND_OPTION_UNRELIABLE 
            || recv_data.packet_type == HAZEL_SEND_OPTION_RELIABLE)
        {
            ret = HAZEL_UDP_CLIENT_RECV_HAS_MESSAGE;
            *out_reader = recv_data.data.msg.reader;
            *out_send_option = recv_data.packet_type;
        }
        else if (recv_data.packet_type == HAZEL_SEND_OPTION_DISCONNECT)
        {
            ret = HAZEL_UDP_CLIENT_RECV_HAS_DISCONNECTED;
            *out_reader = recv_data.data.disconnect.reader;
            *out_send_option = recv_data.packet_type;
        }
        else
        {
            ret = HAZEL_UDP_CLIENT_RECV_NO_MESSAGE;
        }
    }
    else
    {
        ret = HAZEL_UDP_CLIENT_RECV_NO_MESSAGE;
    }

    hazel_udp_connection_manage_reliable(&client->udp_connection);

    return ret;
}
