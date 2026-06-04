#ifndef BSP_DISPLAY_H
#define BSP_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>

void BSP_Display_Init(void);
void BSP_Display_Update(void);
void BSP_Display_ShowStatus(const char* line1, const char* line2);

#endif