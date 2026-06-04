#include "bsp_led.h"
#include "led_strip.h"

#define LED_GPIO_PIN 48
#define BRIGHTNESS_DIVIDER 4 // WS2812s are blindingly bright. Divide intensity by 4.

static led_strip_handle_t led_strip;
static int16_t current_r = -1, current_g = -1, current_b = -1;

void BSP_LED_Init(void) 
{
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO_PIN,
        .max_leds = 1,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // <-- API update for v3.x
        .led_model = LED_MODEL_WS2812,
        .flags = {
            .invert_out = false,
        }
    };

    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };

    led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    led_strip_clear(led_strip);
}

void BSP_LED_SetColor(uint8_t r, uint8_t g, uint8_t b) 
{
    // Prevent spamming the RMT peripheral if the color hasn't changed
    if (current_r == r && current_g == g && current_b == b) return;
    
    if (r == 0 && g == 0 && b == 0) {
        led_strip_clear(led_strip);
    } else {
        led_strip_set_pixel(led_strip, 0, r / BRIGHTNESS_DIVIDER, g / BRIGHTNESS_DIVIDER, b / BRIGHTNESS_DIVIDER);
        led_strip_refresh(led_strip);
    }
    
    current_r = r; current_g = g; current_b = b;
}