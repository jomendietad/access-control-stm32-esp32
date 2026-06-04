#include "app/app.h"
#include "bsp/bsp_placa.h"

#define RELAY_OPEN_TIME_MS 3000
#define DISPLAY_REFRESH_MS  100

typedef enum {
    STATE_IDLE,
    STATE_ACCESS_GRANTED,
    STATE_ACCESS_DENIED
} AccessState_t;

static AccessState_t current_state = STATE_IDLE;
static AccessState_t previous_state = STATE_ACCESS_DENIED; 
static uint32_t relay_open_timestamp = 0;
static uint32_t last_display_update = 0;

void App_Init(void) {
    BSP_Init();
    /* Forzamos una actualización inicial al arrancar */
    last_display_update = BSP_GetTicks() - DISPLAY_REFRESH_MS;
}

void App_Task(void) {
    /* --- 1. REFRESH DE INTERFAZ (CONTROLADO POR TIEMPO) --- */
    /* Solo refrescamos la pantalla si hay cambio de estado O si pasó el tiempo mínimo */
    uint32_t current_tick = BSP_GetTicks();
    if ((current_state != previous_state) || ((current_tick - last_display_update) >= DISPLAY_REFRESH_MS)) {
        
        if (current_state != previous_state) {
            switch (current_state) {
                case STATE_IDLE:
                    BSP_Display_ShowStatus("SISTEMA LISTO", "Acerque rostro/RFID");
                    break;
                case STATE_ACCESS_GRANTED:
                    BSP_Display_ShowStatus("ACCESO CONCEDIDO", "Puerta Abierta...");
                    break;
                case STATE_ACCESS_DENIED:
                    BSP_Display_ShowStatus("ACCESO DENEGADO", "Rostro Desconocido");
                    break;
            }
            previous_state = current_state;
        }
        last_display_update = current_tick;
    }

    /* --- 2. MÁQUINA DE ESTADOS LÓGICA --- */
    switch (current_state) {
        case STATE_IDLE: {
            bool condition_a = false;
            bool condition_b = false;
            char esp32_verdict = BSP_UART_GetFaceVerdict();

            /* Evaluación de Rostro + PIR */
            if (esp32_verdict == 'K') { 
                if (BSP_GPIO_ReadPIR()) {
                    condition_a = true;
                }
            } else if (esp32_verdict == 'U') {
                /* Rostro no reconocido */
                current_state = STATE_ACCESS_DENIED;
                relay_open_timestamp = current_tick;
            }

            /* Evaluación RFID */
            if (BSP_RFID_IsTagValid()) {
                condition_b = true;
            }

            /* Otorgar Acceso */
            if (condition_a || condition_b) {
                BSP_GPIO_SetRelay(true);
                relay_open_timestamp = current_tick;
                current_state = STATE_ACCESS_GRANTED;
            }
            break;
        }
        
        case STATE_ACCESS_GRANTED: {
            if ((current_tick - relay_open_timestamp) >= RELAY_OPEN_TIME_MS) {
                BSP_GPIO_SetRelay(false);
                current_state = STATE_IDLE;
            }
            break;
        }

        case STATE_ACCESS_DENIED: {
            /* Error mostrado por 2 segundos */
            if ((current_tick - relay_open_timestamp) >= 2000) {
                current_state = STATE_IDLE;
            }
            break;
        }
    }
}