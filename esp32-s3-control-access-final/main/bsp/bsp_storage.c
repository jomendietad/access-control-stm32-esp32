#include "bsp/bsp_storage.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#define NVS_NAMESPACE "biometrics"
#define NVS_KEY_TEMPLATE "face_01"

void BSP_Storage_Init(void) 
{
    // Initialize NVS. Handled broadly here, but typically main() calls nvs_flash_init()
    // It's safe to call it multiple times if already initialized.
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
}

bool BSP_Storage_LoadTemplate(uint8_t* template_buffer, size_t size) 
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) return false;

    size_t required_size = size;
    err = nvs_get_blob(my_handle, NVS_KEY_TEMPLATE, template_buffer, &required_size);
    nvs_close(my_handle);
    
    return (err == ESP_OK && required_size == size);
}

void BSP_Storage_SaveTemplate(const uint8_t* template_buffer, size_t size) 
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        nvs_set_blob(my_handle, NVS_KEY_TEMPLATE, template_buffer, size);
        nvs_commit(my_handle); // Force physical write to Flash
        nvs_close(my_handle);
    }
}

void BSP_Storage_Clear(void) 
{
    nvs_handle_t my_handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle) == ESP_OK) {
        nvs_erase_key(my_handle, NVS_KEY_TEMPLATE);
        nvs_commit(my_handle);
        nvs_close(my_handle);
    }
}