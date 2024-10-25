#include <stdint.h>
#include <stddef.h> 
#include <stdio.h>
#include <malloc.h>

#include "hazel/udp/connection.h"
#include "hazel/udp/client.h"

#include "hazel/reader.h"
#include "hazel/writer.h"

#if defined(_WIN32)
#   include <winsock2.h>
#   pragma comment(lib, "Ws2_32.lib")
#endif

#define ARRAY_LENGTH(arr) sizeof(arr) / sizeof(arr[0])

int sizetest();
int sockettest();
int clienttest();

int main()
{
#if defined(_WIN32)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 0), &wsaData);
#endif

    // return sockettest();
    // return clienttest();
    return sizetest();
}

int sizetest()
{
    uint8_t msg[1024] = {0};

    hazel_message_reader reader;
    hazel_message_reader_init(&reader, msg, sizeof(msg), 0);

    if (!hazel_message_reader_has_remaining(&reader, 1024))
    {
        printf("Heh\n");
        return -1;
    }

    if (hazel_message_reader_has_remaining(&reader, 1025))
    {
        printf("Heh\n");
        return -1;
    }

    return 0;
}

int clienttest()
{
    int ret;

    hazel_udp_client client;
    
    if ((ret = hazel_udp_client_init(&client, "127.0.0.1", 6969, HAZEL_IP_MODE_IPV4)) != 0)
    {
        printf("client init fail %d\n", ret);
        return ret;
    }

    printf("client init done\n");

    hazel_message_writer handshake_writer;
    hazel_message_writer_init_malloc(&handshake_writer, 32);
    hazel_message_writer_start_message(&handshake_writer, 0x69);
    hazel_message_writer_int32(&handshake_writer, 6969);
    hazel_message_writer_end_message(&handshake_writer);

    HAZEL_LOG_DEBUG_PRINT_BYTES("handshake_writer", handshake_writer.data, handshake_writer.size, 0);

    if ((ret = hazel_udp_client_handshake(&client, handshake_writer.data, handshake_writer.size)) != HAZEL_UDP_CLIENT_HANDSHAKE_SUCCESS)
    {
        printf("handshake fail %d\n", ret);
        return ret;
    }

    hazel_message_writer_free(&handshake_writer);

    printf("handshake done\n");

    int ticks = 0;

    hazel_message_reader reader;
    enum hazel_send_option send_option;
    while(true)
    {
        ret = hazel_udp_client_recv(&client, &send_option, &reader);
        if (ret == HAZEL_UDP_CLIENT_RECV_NO_MESSAGE)
        {
            continue;
        }

        printf("recv exit: %d\n", ret);
        if(ret == HAZEL_UDP_CLIENT_RECV_HAS_MESSAGE)
        {
            printf("received!\n");

            uint32_t pong_count;
            hazel_message_reader_packed_uint32(&reader, &pong_count);
            printf("Pongs: %d\n", pong_count);

            char* str;
            hazel_message_reader_string_malloc(&reader, &str);
            printf("message: %s\n", str);
            hazel_message_reader_free(&reader);
            free(str);

            uint8_t buffer[1024];
            hazel_message_writer writer;
            hazel_message_writer_init(&writer, buffer, sizeof(buffer));
            hazel_message_writer_clear(&writer, HAZEL_SEND_OPTION_UNRELIABLE);
            hazel_message_writer_packed_uint32(&writer, pong_count + 1);

            hazel_udp_connection_send(&client.udp_connection, &writer);

        }
    }

    printf("bye\n");

    hazel_udp_client_free(&client);

    return 0;
}