#ifndef BSP_PLACA_H
#define BSP_PLACA_H

#include "bsp_uart.h"
#include "bsp_storage.h"
#include "bsp_tcp.h"
#include "bsp_wifi.h"
#include "bsp_led.h"
#include "bsp_web.h"
#include "bsp_os.h"
#include <stdint.h>
#include <stdbool.h>

#define BSP_WAIT_FOREVER 0xFFFFFFFF

typedef void* BSP_MutexHandle_t;

BSP_MutexHandle_t BSP_OS_MutexCreate(void);
bool BSP_OS_MutexTake(BSP_MutexHandle_t mutex, uint32_t timeout_ms);
void BSP_OS_MutexGive(BSP_MutexHandle_t mutex);
void BSP_OS_StartVisionTask(void (*task_func)(void*));
void BSP_OS_Delay(uint32_t ms);

#endif /* BSP_PLACA_H */