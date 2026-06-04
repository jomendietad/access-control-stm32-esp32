#include "app/app.h"

void app_main(void) {
    App_Init();

    while(1) {
        App_Task();
        App_Yield();
    }
}