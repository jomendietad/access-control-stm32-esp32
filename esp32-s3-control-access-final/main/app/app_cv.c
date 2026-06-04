#include "app/app_cv.h"
#include "bsp/bsp_placa.h" 
#include <string.h>

#define RECOGNITION_THRESHOLD 150000 
#define TEMPLATE_SIZE 100

static uint8_t known_user_template[TEMPLATE_SIZE];
static bool is_user_registered = false;
static bool cv_is_initialized = false;
static int training_frames = 0;

static void draw_bounding_box(uint16_t* fb, int width, int height, int x, int y, int w, int h, uint16_t color) {
    if (x < 0) x = 0; 
    if (y < 0) y = 0;
    
    if (x + w > width) w = width - x;
    if (y + h > height) h = height - y;

    for (int i = x; i < x + w; i++) {
        fb[y * width + i] = color;
        fb[(y + h - 1) * width + i] = color;
    }
    for (int j = y; j < y + h; j++) {
        fb[j * width + x] = color;
        fb[j * width + x + w - 1] = color;
    }
}

static void extract_face_features(uint16_t* fb, int f_width, int box_x, int box_y, int box_w, int box_h, uint8_t* output_template) {
    float step_x = (float)box_w / 10.0f;
    float step_y = (float)box_h / 10.0f;

    for (int ty = 0; ty < 10; ty++) {
        for (int tx = 0; tx < 10; tx++) {
            int px_x = box_x + (int)(tx * step_x);
            int px_y = box_y + (int)(ty * step_y);
            uint16_t px = fb[px_y * f_width + px_x];
            
            uint8_t r = ((px >> 11) & 0x1F) << 3;
            uint8_t g = ((px >> 5) & 0x3F) << 2;
            uint8_t b = (px & 0x1F) << 3;
            output_template[ty * 10 + tx] = (uint8_t)((r*3 + g*4 + b*1) >> 3);
        }
    }
}

void App_CV_ResetMemory(void) 
{
    BSP_Storage_Clear();
    is_user_registered = false;
    training_frames = 0;
    memset(known_user_template, 0, TEMPLATE_SIZE);
    BSP_LED_SetColor(0, 0, 0); // Turn off LED on reset
}

void App_CV_ProcessFrame(uint8_t* frame_buffer, int width, int height, bool motion_detected) 
{
    if (!cv_is_initialized) {
        BSP_Storage_Init();
        if (BSP_Storage_LoadTemplate(known_user_template, TEMPLATE_SIZE)) {
            is_user_registered = true; 
        }
        cv_is_initialized = true;
    }

    uint16_t* pixels = (uint16_t*)frame_buffer;
    int min_x = width, min_y = height, max_x = 0, max_y = 0;
    int skin_pixels_count = 0;
    bool face_is_tracked = false; // Tracks if a face is present in the current frame

    for (int y = 0; y < height; y+=2) { 
        for (int x = 0; x < width; x+=2) {
            uint16_t px = pixels[y * width + x];
            uint8_t r5 = (px >> 11) & 0x1F, g6 = (px >> 5) & 0x3F, b5 = px & 0x1F;
            uint8_t r = (r5 << 3) | (r5 >> 2), g = (g6 << 2) | (g6 >> 4), b = (b5 << 3) | (b5 >> 2);

            int r_g_diff = (r > g) ? (r - g) : (g - r);
            if (r > 95 && g > 40 && b > 20 && (r > g) && (r > b) && (r_g_diff > 15)) {
                if (x < min_x) min_x = x; 
                if (y < min_y) min_y = y;
                if (x > max_x) max_x = x; 
                if (y > max_y) max_y = y;
                skin_pixels_count++;
            }
        }
    }

    if (skin_pixels_count > 100 && max_x > min_x && max_y > min_y) 
    {
        int box_w = max_x - min_x;
        int box_h = max_y - min_y;
        
        if (box_w > 20 && box_h > 20) 
        {
            face_is_tracked = true;
            uint8_t current_features[100];
            extract_face_features(pixels, width, min_x, min_y, box_w, box_h, current_features);

            if (!is_user_registered) 
            {
                training_frames++;
                if (training_frames > 5) {
                    memcpy(known_user_template, current_features, TEMPLATE_SIZE);
                    is_user_registered = true;
                    BSP_Storage_SaveTemplate(known_user_template, TEMPLATE_SIZE);
                }
                
                BSP_LED_SetColor(0, 0, 255); // Blue (Registering)
                draw_bounding_box(pixels, width, height, min_x, min_y, box_w, box_h, 0x001F);
            } 
            else 
            {
                int difference = 0;
                for (int i = 0; i < 100; i++) {
                    int diff = current_features[i] - known_user_template[i];
                    difference += (diff * diff); 
                }

                bool face_is_known = (difference < RECOGNITION_THRESHOLD);
                bool access_granted = (face_is_known && motion_detected);
                /* Notify external MCU via UART immediately after inference */
                BSP_UART_SendResult(face_is_known);
                uint16_t box_color;

                if (access_granted) {
                    box_color = 0x07E0;
                    BSP_LED_SetColor(0, 255, 0); // Green (Access Granted)
                } else if (face_is_known) {
                    box_color = 0xFFE0;
                    BSP_LED_SetColor(255, 255, 0); // Yellow (Waiting for PIR)
                } else {
                    box_color = 0xF800;
                    BSP_LED_SetColor(255, 0, 0); // Red (Unknown Face)
                }

                draw_bounding_box(pixels, width, height, min_x, min_y, box_w, box_h, box_color);
                
                char label = access_granted ? 'A' : (face_is_known ? 'W' : 'U');
                if (label == 'A') {
                    for(int i=0; i<7; i++) { pixels[(min_y-8+i)*width + min_x] = box_color; pixels[(min_y-8+i)*width + min_x+4] = box_color; }
                    pixels[(min_y-8)*width + min_x+1] = box_color; pixels[(min_y-8)*width + min_x+2] = box_color; pixels[(min_y-8)*width + min_x+3] = box_color;
                    pixels[(min_y-4)*width + min_x+1] = box_color; pixels[(min_y-4)*width + min_x+2] = box_color; pixels[(min_y-4)*width + min_x+3] = box_color;
                } else if (label == 'W') {
                    for(int i=0; i<7; i++) { pixels[(min_y-8+i)*width + min_x] = box_color; pixels[(min_y-8+i)*width + min_x+4] = box_color; }
                    pixels[(min_y-1)*width + min_x+1] = box_color; pixels[(min_y-1)*width + min_x+2] = box_color; pixels[(min_y-1)*width + min_x+3] = box_color;
                    pixels[(min_y-5)*width + min_x+2] = box_color; 
                } else {
                    for(int i=0; i<7; i++) { pixels[(min_y-8+i)*width + min_x] = box_color; pixels[(min_y-8+i)*width + min_x+4] = box_color; }
                    for(int i=1; i<4; i++) pixels[(min_y-1)*width + min_x+i] = box_color;
                }
            }
        }
    }

    // If nobody is in front of the camera, turn off the LED to save power and be discreet
    if (!face_is_tracked) {
        BSP_LED_SetColor(0, 0, 0);
    }
}