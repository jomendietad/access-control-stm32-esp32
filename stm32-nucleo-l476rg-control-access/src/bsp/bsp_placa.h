#ifndef BSP_PLACA_H
#define BSP_PLACA_H

#include <stdint.h>
#include <stdbool.h>
#include "bsp_uart.h"
#include "bsp_gpio.h"
#include "bsp_rfid.h"
#include "bsp_display.h"
#include "stm32l4xx_hal.h"

void BSP_Init(void);
uint32_t BSP_GetTicks(void);

/* Hardware Interfaces */
bool BSP_GPIO_ReadPIR(void);
void BSP_GPIO_SetRelay(bool state);
char BSP_UART_GetFaceVerdict(void);
bool BSP_RFID_IsTagValid(void);

#endif