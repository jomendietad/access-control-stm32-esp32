#include "bsp/bsp_placa.h"
#include "bsp/bsp_gpio.h"

#define PIR_PIN    GPIO_PIN_0
#define PIR_PORT   GPIOA
#define RELAY_PIN  GPIO_PIN_1
#define RELAY_PORT GPIOA

void BSP_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* PIR Sensor Input */
    GPIO_InitStruct.Pin = PIR_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(PIR_PORT, &GPIO_InitStruct);

    /* Relay Output */
    GPIO_InitStruct.Pin = RELAY_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RELAY_PORT, &GPIO_InitStruct);
    
    HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, GPIO_PIN_RESET);
}

bool BSP_GPIO_ReadPIR(void) {
    return (HAL_GPIO_ReadPin(PIR_PORT, PIR_PIN) == GPIO_PIN_SET);
}

void BSP_GPIO_SetRelay(bool state) {
    HAL_GPIO_WritePin(RELAY_PORT, RELAY_PIN, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}