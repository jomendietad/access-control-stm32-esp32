#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef int BSP_TCP_SocketHandle_t;

BSP_TCP_SocketHandle_t BSP_TCP_Connect(const char* ip, uint16_t port);
bool BSP_TCP_ReadExact(BSP_TCP_SocketHandle_t sock, uint8_t* buffer, uint32_t length);
void BSP_TCP_Close(BSP_TCP_SocketHandle_t sock);
bool BSP_TCP_WriteExact(BSP_TCP_SocketHandle_t sock, const uint8_t* buffer, uint32_t length);