#ifndef BSP_OS_H
#define BSP_OS_H

#include <stdint.h>
#include <stdbool.h>

#define BSP_WAIT_FOREVER 0xFFFFFFFF

typedef void* BSP_MutexHandle_t;
typedef void* BSP_SemaphoreHandle_t; /* Event flag handle */

/* Mutex API */
BSP_MutexHandle_t BSP_OS_MutexCreate(void);
bool BSP_OS_MutexTake(BSP_MutexHandle_t mutex, uint32_t timeout_ms);
void BSP_OS_MutexGive(BSP_MutexHandle_t mutex);

/* Semaphore API (Non-blocking Event Triggers) */
BSP_SemaphoreHandle_t BSP_OS_SemaphoreCreateBinary(void);
bool BSP_OS_SemaphoreTake(BSP_SemaphoreHandle_t sem, uint32_t timeout_ms);
void BSP_OS_SemaphoreGive(BSP_SemaphoreHandle_t sem);

/* System Timing */
uint32_t BSP_OS_GetTicks(void);
void BSP_OS_Delay(uint32_t ms); 
void BSP_OS_StartVisionTask(void (*task_func)(void*));

#endif /* BSP_OS_H */