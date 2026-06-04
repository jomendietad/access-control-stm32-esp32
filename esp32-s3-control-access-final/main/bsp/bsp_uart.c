#include "bsp_uart.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <string.h>

void BSP_UART_Init(void) {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_PORT_NUM, 256, 0, 0, NULL, 0);
}

void BSP_UART_SendResult(bool is_known) {
    const char* msg = is_known ? "K\n" : "U\n";
    uart_write_bytes(UART_PORT_NUM, msg, strlen(msg));
}

bool BSP_UART_ReadTest(char* rx_buffer, size_t max_len) {
    // Wait up to 100ms for the loopback data to arrive
    int len = uart_read_bytes(UART_PORT_NUM, rx_buffer, max_len - 1, pdMS_TO_TICKS(100));
    if (len > 0) {
        rx_buffer[len] = '\0';
        return true;
    }
    return false;
}