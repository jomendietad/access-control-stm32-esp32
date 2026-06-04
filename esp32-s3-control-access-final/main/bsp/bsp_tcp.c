#include "bsp/bsp_tcp.h"
#include "lwip/sockets.h"

BSP_TCP_SocketHandle_t BSP_TCP_Connect(const char* ip, uint16_t port) 
{
    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = inet_addr(ip);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) return -1;

    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) 
    {
        close(sock);
        return -1;
    }
    return sock;
}

bool BSP_TCP_ReadExact(BSP_TCP_SocketHandle_t sock, uint8_t* buffer, uint32_t length) 
{
    uint32_t bytes_received = 0;
    while (bytes_received < length) 
    {
        int len = recv(sock, buffer + bytes_received, length - bytes_received, 0);
        if (len <= 0) return false;
        bytes_received += len;
    }
    return true;
}

void BSP_TCP_Close(BSP_TCP_SocketHandle_t sock) 
{
    close(sock);
}

bool BSP_TCP_WriteExact(BSP_TCP_SocketHandle_t sock, const uint8_t* buffer, uint32_t length) 
{
    uint32_t bytes_sent = 0;
    while (bytes_sent < length) 
    {
        int sent = send(sock, buffer + bytes_sent, length - bytes_sent, 0);
        if (sent <= 0) return false;
        bytes_sent += sent;
    }
    return true;
}