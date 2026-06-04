#pragma once
#include <stdint.h>
#include <stdbool.h>

void BSP_Storage_Init(void);
bool BSP_Storage_LoadTemplate(uint8_t* template_buffer, size_t size);
void BSP_Storage_SaveTemplate(const uint8_t* template_buffer, size_t size);
void BSP_Storage_Clear(void);