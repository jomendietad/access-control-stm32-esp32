#ifndef BSP_UART_H
#define BSP_UART_H

#include <stdint.h>
#include <stdbool.h>

#define UART_PORT_NUM      1
#define UART_BAUD_RATE     115200
#define UART_TX_PIN        17 
#define UART_RX_PIN        18 

void BSP_UART_Init(void);
void BSP_UART_SendResult(bool is_known);
bool BSP_UART_ReadTest(char* rx_buffer, size_t max_len);

#endif // BSP_UART_H