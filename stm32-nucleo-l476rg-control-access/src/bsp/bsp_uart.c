#include "bsp/bsp_placa.h"
#include "bsp/bsp_uart.h"
#include <string.h>

static UART_HandleTypeDef huart1; /* Para la ESP32 */
static UART_HandleTypeDef huart2; /* Para la PC (Monitor Serial) */

static uint8_t rx_byte = 0;
static char last_verdict = 'N'; /* Inicializado en 'N' (None) */

void BSP_UART_Init(void) {
    /* 1. Habilitar Relojes */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 2. Configurar Pines para USART2 (PC) -> PA2 (TX), PA3 (RX) */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3; 
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 3. Configurar Pines para USART1 (ESP32) -> PA9 (TX), PA10 (RX) */
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10; 
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* 4. Inicializar USART2 (PC) */
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX; /* TX y RX habilitados para la PC */
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);

    /* 5. Inicializar USART1 (ESP32) */
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_RX; /* Solo necesitamos recibir de la ESP32 */
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart1);

    /* 6. Configurar Interrupciones SOLO para USART1 (ESP32) */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);

    /* Iniciar la recepción por interrupción en USART1 */
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
}

/* Manejador de interrupción para USART1 (ESP32) */
void USART1_IRQHandler(void) {
    HAL_UART_IRQHandler(&huart1);
}

/* Callback cuando se recibe un byte */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART1) {
        if (rx_byte == 'K' || rx_byte == 'U') {
            last_verdict = (char)rx_byte;
        }
        /* Volver a armar la interrupción para el siguiente byte */
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
}

char BSP_UART_GetFaceVerdict(void) {
    char verdict = last_verdict;
    last_verdict = 'N'; /* Limpiar después de leer */
    return verdict;
}

void BSP_UART_SendString(const char* str) {
    /* Enviar a la PC usando USART2 */
    HAL_UART_Transmit(&huart2, (uint8_t*)str, strlen(str), 100);
}