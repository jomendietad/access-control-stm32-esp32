#include "bsp_os.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

BSP_MutexHandle_t BSP_OS_MutexCreate(void) {
    return (BSP_MutexHandle_t)xSemaphoreCreateMutex();
}

bool BSP_OS_MutexTake(BSP_MutexHandle_t mutex, uint32_t timeout_ms) {
    TickType_t ticks = (timeout_ms == BSP_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xSemaphoreTake((SemaphoreHandle_t)mutex, ticks) == pdTRUE);
}

void BSP_OS_MutexGive(BSP_MutexHandle_t mutex) {
    xSemaphoreGive((SemaphoreHandle_t)mutex);
}

BSP_SemaphoreHandle_t BSP_OS_SemaphoreCreateBinary(void) {
    return (BSP_SemaphoreHandle_t)xSemaphoreCreateBinary();
}

bool BSP_OS_SemaphoreTake(BSP_SemaphoreHandle_t sem, uint32_t timeout_ms) {
    TickType_t ticks = (timeout_ms == BSP_WAIT_FOREVER) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return (xSemaphoreTake((SemaphoreHandle_t)sem, ticks) == pdTRUE);
}

void BSP_OS_SemaphoreGive(BSP_SemaphoreHandle_t sem) {
    xSemaphoreGive((SemaphoreHandle_t)sem);
}

uint32_t BSP_OS_GetTicks(void) {
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

void BSP_OS_Delay(uint32_t ms) {
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void BSP_OS_StartVisionTask(void (*task_func)(void*)) {
    xTaskCreatePinnedToCore(task_func, "VisionTask", 8192, NULL, 5, NULL, 1);
}