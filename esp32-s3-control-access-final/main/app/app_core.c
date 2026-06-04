#include "app.h"
#include "app_cv.h"
#include "bsp/bsp_placa.h" 
#include "secrets/secrets.h" 
#include <stdlib.h>
#include <string.h>

#define FRAME_WIDTH  160
#define FRAME_HEIGHT 120
#define FRAME_SIZE   (FRAME_WIDTH * FRAME_HEIGHT * 2) 

#define TCP_SERVER_PORT 5001
#define TCP_RETURN_PORT 5002
#define TCP_SYNC_WORD 0xAA55

#pragma pack(push, 1) /* Fuerza al compilador a no dejar bytes vacíos (padding) */
typedef struct {
    uint16_t sync_word;
    uint32_t payload_size;
} TCP_FrameHeader_t;
#pragma pack(pop)

static uint8_t *psram_frame_buffer = NULL;
static BSP_MutexHandle_t frame_mutex = NULL;
static BSP_SemaphoreHandle_t frame_ready_sem = NULL; /* Event-driven trigger */

static BSP_TCP_SocketHandle_t tcp_sock = -1;
static BSP_TCP_SocketHandle_t tcp_tx_sock = -1;

static void App_VisionTask(void *params) {
    uint8_t *local_buffer = (uint8_t *)malloc(FRAME_SIZE);
    
    while(1) {
        if (tcp_tx_sock < 0 && BSP_WiFi_IsConnected()) {
            tcp_tx_sock = BSP_TCP_Connect(FEDORA_SERVER_IP, TCP_RETURN_PORT); 
        }

        /* Pure event-driven sleep. Awakes ONLY when Core 0 signals a full frame */
        if (BSP_OS_SemaphoreTake(frame_ready_sem, BSP_WAIT_FOREVER)) {
            
            if (BSP_OS_MutexTake(frame_mutex, BSP_WAIT_FOREVER)) {
                memcpy(local_buffer, psram_frame_buffer, FRAME_SIZE);
                BSP_OS_MutexGive(frame_mutex);
            }
            
            /* Execute AI Pipeline */
            App_CV_ProcessFrame(local_buffer, FRAME_WIDTH, FRAME_HEIGHT, true); 
            
            /* Dispatch UART to STM32F0 */
            BSP_UART_SendResult(false);

            /* Stream back to Fedora */
            if (tcp_tx_sock >= 0) {
                if (!BSP_TCP_WriteExact(tcp_tx_sock, local_buffer, FRAME_SIZE)) {
                    BSP_TCP_Close(tcp_tx_sock);
                    tcp_tx_sock = -1;
                }
            }
        }
    }
}

void App_Init(void) {
    BSP_Storage_Init();
    BSP_UART_Init();
    BSP_WiFi_Init();

    psram_frame_buffer = (uint8_t *)malloc(FRAME_SIZE); 
    frame_mutex = BSP_OS_MutexCreate();
    frame_ready_sem = BSP_OS_SemaphoreCreateBinary(); 

    BSP_OS_StartVisionTask(App_VisionTask);
}

void App_Task(void) {
    static bool server_started = false;
    static uint32_t last_reconnect_tick = 0; /* Non-blocking state machine */

    if (BSP_WiFi_IsConnected()) {
        if (!server_started) {
            BSP_Web_StartServer();
            server_started = true;
        }
        
        if (BSP_Web_IsResetRequested()) {
            App_CV_ResetMemory();
            BSP_Web_ClearResetRequest();
        }

        if (tcp_sock < 0) {
            /* 2-Second Non-Blocking Reconnect Window */
            if ((BSP_OS_GetTicks() - last_reconnect_tick) > 2000) {
                tcp_sock = BSP_TCP_Connect(FEDORA_SERVER_IP, TCP_SERVER_PORT); 
                last_reconnect_tick = BSP_OS_GetTicks();
            }
        } else {
            TCP_FrameHeader_t header;
            /* ReadExact halts natively at the socket level without CPU polling */
            if (BSP_TCP_ReadExact(tcp_sock, (uint8_t*)&header, sizeof(TCP_FrameHeader_t))) {
                if (header.sync_word == TCP_SYNC_WORD && header.payload_size == FRAME_SIZE) {
                    uint8_t *temp_recv_buffer = (uint8_t *)malloc(FRAME_SIZE);
                    if (temp_recv_buffer != NULL) {
                        if (BSP_TCP_ReadExact(tcp_sock, temp_recv_buffer, FRAME_SIZE)) {
                            
                            if (BSP_OS_MutexTake(frame_mutex, BSP_WAIT_FOREVER)) { 
                                memcpy(psram_frame_buffer, temp_recv_buffer, FRAME_SIZE);
                                BSP_OS_MutexGive(frame_mutex);
                            }
                            
                            /* Signal the AI Core that data is ready */
                            BSP_OS_SemaphoreGive(frame_ready_sem);

                        } else {
                            BSP_TCP_Close(tcp_sock);
                            tcp_sock = -1;
                        }
                        free(temp_recv_buffer);
                    }
                } else {
                    BSP_TCP_Close(tcp_sock);
                    tcp_sock = -1;
                }
            } else {
                BSP_TCP_Close(tcp_sock);
                tcp_sock = -1;
            }
        }
    } else {
        if (server_started) {
            BSP_Web_StopServer();
            server_started = false;
        }
    }
}

void App_Yield(void) {
    BSP_OS_Delay(10); 
}