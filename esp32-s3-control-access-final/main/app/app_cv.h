#pragma once
#include <stdint.h>
#include <stdbool.h>

void App_CV_ProcessFrame(uint8_t* frame_buffer, int width, int height, bool motion_detected);
void App_CV_ResetMemory(void);