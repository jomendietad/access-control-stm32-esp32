#ifndef BSP_WEB_H
#define BSP_WEB_H

#include <stdbool.h>

void BSP_Web_StartServer(void);
void BSP_Web_StopServer(void);
bool BSP_Web_IsResetRequested(void);
void BSP_Web_ClearResetRequest(void);

#endif /* BSP_WEB_H */