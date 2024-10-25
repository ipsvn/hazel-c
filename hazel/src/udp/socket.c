#include "hazel/udp/socket.h"

#include "../utils.h"

#ifdef HAZEL_NET_USE_POLL
#include <poll.h>
#else

#if !defined(_WIN32)
#include <sys/select.h>
#endif

#endif
#include <errno.h>

#if defined(_WIN32)
#include "mswsock.h"
#include <ws2tcpip.h>
#include <wspiapi.h>
#   pragma comment(lib, "Ws2_32.lib")
#endif

int hazel_ip_mode_to_af (enum hazel_ip_mode ip_mode)
{
    int sock_family = AF_INET;
    switch(ip_mode) {
    case HAZEL_IP_MODE_IPV4:
        sock_family = AF_INET;
        break;
    case HAZEL_IP_MODE_IPV6:
        sock_family = AF_INET6;
        break;
    }
    return sock_family;
}

int hazel_udp_socket_init(hazel_udp_socket* hazel_socket)
{
    hazel_socket->_sock_handle = -1;
    return 0;
}
void hazel_udp_socket_free(hazel_udp_socket* hazel_socket)
{
    HAZEL_UNUSED(hazel_socket);
}

int hazel_udp_socket_open(hazel_udp_socket* hazel_socket, enum hazel_ip_mode ip_mode)
{  
    hazel_socket->ip_mode = ip_mode;
    int family = hazel_ip_mode_to_af(hazel_socket->ip_mode);
    int handle = socket(family, SOCK_DGRAM, 0);

    if (handle == -1)
    {
        return HAZEL_UDP_SOCKET_SOCKET_ERROR;
    }

    hazel_socket->_sock_handle = handle;

#if defined(_WIN32)
    {
        const long off = 0;
        if(ioctlsocket(hazel_socket->_sock_handle, SIO_UDP_CONNRESET, &off) 
            != SOCKET_ERROR)
        {

        }
    }
#endif
    return 0;
}

int hazel_udp_socket_close(hazel_udp_socket* socket)
{
    int ret = close(socket->_sock_handle);
    if (ret == -1)
    {
        return HAZEL_UDP_SOCKET_SOCKET_ERROR;
    }
    return ret;
}

int hazel_udp_socket_connect(hazel_udp_socket* hazel_socket, const char* hostname, int port)
{
    int ret = 0;
    
    struct addrinfo hints, *result, *rp;
    memset (&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if ((ret = getaddrinfo(hostname, NULL, &hints, &result)) != 0) {
        return ret;
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        if (rp->ai_family == AF_INET) {
            memcpy(&hazel_socket->_sa_in, rp->ai_addr, 
                   sizeof(struct sockaddr_in)); 
            break;
        }
    }

    freeaddrinfo(result);

    if (rp == NULL) {
        return -1;
    }

    hazel_socket->_sa_in.sin_port = htons(port);
    if((ret = connect(hazel_socket->_sock_handle, 
            (const struct sockaddr*) &hazel_socket->_sa_in, 
            sizeof(struct sockaddr_in))) 
            != 0) {
        return ret;
    }

    return 0;

}

int hazel_udp_socket_recv(hazel_udp_socket* socket, uint8_t* buffer, 
                              size_t size, 
                              int flags, int timeout)
{
#if HAZEL_NET_USE_POLL
    struct pollfd pfd;
    pfd.fd = socket->_sock_handle;
    pfd.events = POLLIN;

    int ret = poll(&pfd, 1, timeout);

#else

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(socket->_sock_handle, &fdset);

    struct timeval tv;
    tv.tv_sec  = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    int ret = select(socket->_sock_handle + 1, &fdset, NULL, NULL, &tv);

#endif

    if (ret == 0)
    {
        return HAZEL_UDP_SOCKET_RECV_NO_MESSAGE;
    }
    
    if (ret <= -1)
    {
        if (ret == EINTR)
        {
            return HAZEL_UDP_SOCKET_RECV_NO_MESSAGE;
        }
        return HAZEL_UDP_SOCKET_RECV_ERROR;
    }
    
    ret = (int)recv(socket->_sock_handle, buffer, size, flags);
    if (ret == -1)
    {
        switch(errno)
        {
        case ECONNREFUSED:
        {
            return HAZEL_UDP_SOCKET_CONN_REFUSED;
        }
        }
    }
    return ret;
}

int hazel_udp_socket_send(hazel_udp_socket* socket, uint8_t* buffer, size_t size, 
                           int flags)
{
    return (int)send(socket->_sock_handle, buffer, size, flags);
}
