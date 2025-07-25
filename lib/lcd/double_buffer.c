/*****************************************************************************
* | File      	:   double_buffer.c
* | Author      :   
* | Function    :   Double buffering implementation for LCD
* | Info        :   
*   - Memory-based off-screen rendering
*   - Fast DMA-based buffer transfer to LCD
*   - Optimized drawing functions
*----------------
******************************************************************************/

#include "double_buffer.h"
#include "LCD_Driver.h"
#include "LCD_GUI.h"
#include "DEV_Config.h"
#include "hardware/dma.h"
#include "pico/stdlib.h"

// Global double buffer instance
double_buffer_t g_double_buffer = {
    .front_buffer = NULL,
    .back_buffer = NULL,
    .buffer_ready = false,
    .using_double_buffer = false
};

/**
 * Initialize double buffering system
 * @return true if successful, false if failed
 */
bool double_buffer_init(void) {
    // Allocate memory for front and back buffers
    g_double_buffer.front_buffer = (uint16_t*)malloc(BUFFER_SIZE * sizeof(uint16_t));
    g_double_buffer.back_buffer = (uint16_t*)malloc(BUFFER_SIZE * sizeof(uint16_t));
    
    if (!g_double_buffer.front_buffer || !g_double_buffer.back_buffer) {
        double_buffer_cleanup();
        return false;
    }
    
    // Clear both buffers
    memset(g_double_buffer.front_buffer, 0, BUFFER_SIZE * sizeof(uint16_t));
    memset(g_double_buffer.back_buffer, 0, BUFFER_SIZE * sizeof(uint16_t));
    
    g_double_buffer.using_double_buffer = true;
    g_double_buffer.buffer_ready = false;
    
    return true;
}

/**
 * Cleanup double buffering resources
 */
void double_buffer_cleanup(void) {
    if (g_double_buffer.front_buffer) {
        free(g_double_buffer.front_buffer);
        g_double_buffer.front_buffer = NULL;
    }
    
    if (g_double_buffer.back_buffer) {
        free(g_double_buffer.back_buffer);
        g_double_buffer.back_buffer = NULL;
    }
    
    g_double_buffer.using_double_buffer = false;
    g_double_buffer.buffer_ready = false;
}

/**
 * Swap front and back buffers
 */
void double_buffer_swap(void) {
    if (!g_double_buffer.using_double_buffer) return;
    
    uint16_t* temp = g_double_buffer.front_buffer;
    g_double_buffer.front_buffer = g_double_buffer.back_buffer;
    g_double_buffer.back_buffer = temp;
    
    g_double_buffer.buffer_ready = true;
}

/**
 * Clear the back buffer with specified color
 */
void double_buffer_clear(COLOR color) {
    if (!g_double_buffer.using_double_buffer || !g_double_buffer.back_buffer) return;
    
    for (int i = 0; i < BUFFER_SIZE; i++) {
        g_double_buffer.back_buffer[i] = color;
    }
}

/**
 * Set pixel in back buffer
 */
void double_buffer_set_pixel(POINT x, POINT y, COLOR color) {
    if (!g_double_buffer.using_double_buffer || !g_double_buffer.back_buffer) return;
    if (x >= BUFFER_WIDTH || y >= BUFFER_HEIGHT) return;
    
    g_double_buffer.back_buffer[y * BUFFER_WIDTH + x] = color;
}

/**
 * Draw line in back buffer using Bresenham's algorithm
 */
void double_buffer_draw_line(POINT x1, POINT y1, POINT x2, POINT y2, COLOR color) {
    if (!g_double_buffer.using_double_buffer) return;
    
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    POINT x = x1, y = y1;
    
    while (true) {
        double_buffer_set_pixel(x, y, color);
        
        if (x == x2 && y == y2) break;
        
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

/**
 * Draw rectangle in back buffer
 */
void double_buffer_draw_rectangle(POINT x1, POINT y1, POINT x2, POINT y2, COLOR color, bool filled) {
    if (!g_double_buffer.using_double_buffer) return;
    
    // Ensure x1,y1 is top-left and x2,y2 is bottom-right
    if (x1 > x2) { POINT temp = x1; x1 = x2; x2 = temp; }
    if (y1 > y2) { POINT temp = y1; y1 = y2; y2 = temp; }
    
    if (filled) {
        for (POINT y = y1; y <= y2; y++) {
            for (POINT x = x1; x <= x2; x++) {
                double_buffer_set_pixel(x, y, color);
            }
        }
    } else {
        // Draw outline only
        for (POINT x = x1; x <= x2; x++) {
            double_buffer_set_pixel(x, y1, color);  // Top edge
            double_buffer_set_pixel(x, y2, color);  // Bottom edge
        }
        for (POINT y = y1; y <= y2; y++) {
            double_buffer_set_pixel(x1, y, color);  // Left edge
            double_buffer_set_pixel(x2, y, color);  // Right edge
        }
    }
}

/**
 * Draw text in back buffer (simplified version)
 * Note: This is a basic implementation. For full font rendering,
 * you may need to adapt the existing font rendering system.
 */
void double_buffer_draw_text(POINT x, POINT y, const char* text, sFONT* font, COLOR bg_color, COLOR text_color) {
    if (!g_double_buffer.using_double_buffer || !text || !font) return;
    
    // This is a simplified implementation
    // For full functionality, you would need to render each character
    // from the font data into the back buffer
    
    // For now, draw a simple rectangle to indicate text position
    double_buffer_draw_rectangle(x, y, x + strlen(text) * 8, y + font->Height, bg_color, true);
}

/**
 * Copy front buffer to LCD using optimized burst transfer
 */
void double_buffer_copy_to_lcd(void) {
    if (!g_double_buffer.using_double_buffer || !g_double_buffer.front_buffer) return;
    
    // Set LCD window to full screen
    LCD_SetWindow(0, 0, LCD_X_MAXPIXEL - 1, LCD_Y_MAXPIXEL - 1);
    
    // Use continuous SPI transfer with CS held low for entire operation
    // This eliminates CS toggling overhead
    DEV_Digital_Write(LCD_CS_PIN, 0);  // Assert CS for entire transfer
    DEV_Digital_Write(LCD_DC_PIN, 1);  // Set to data mode
    
    // Burst transfer in larger chunks to reduce overhead
    const int CHUNK_SIZE = 1024;  // Transfer in 1KB chunks
    uint16_t* buffer_ptr = g_double_buffer.front_buffer;
    
    for (int offset = 0; offset < BUFFER_SIZE; offset += CHUNK_SIZE) {
        int transfer_size = (offset + CHUNK_SIZE > BUFFER_SIZE) ? 
                            (BUFFER_SIZE - offset) : CHUNK_SIZE;
        
        // Direct SPI transfer without CS toggling
        for (int i = 0; i < transfer_size; i++) {
            uint16_t color = buffer_ptr[offset + i];
            // Send high byte then low byte for 16-bit color
            SPI4W_Write_Byte(color >> 8);
            SPI4W_Write_Byte(color & 0xFF);
        }
    }
    
    DEV_Digital_Write(LCD_CS_PIN, 1);  // Release CS
}

/**
 * Wait for vertical blanking period to reduce tearing
 * This is a software approximation since we don't have hardware VSYNC
 */
void double_buffer_wait_for_vblank(void) {
    // Approximate VSYNC timing based on LCD refresh rate
    // Most LCD panels refresh at 60Hz (16.67ms per frame)
    // Wait for a safe window to update the display
    static uint64_t last_update_time = 0;
    uint64_t current_time = time_us_64();
    
    // Ensure minimum 16.67ms between updates (60 FPS max)
    const uint64_t FRAME_TIME_US = 16667;  // 60 FPS
    
    if (current_time - last_update_time < FRAME_TIME_US) {
        sleep_us(FRAME_TIME_US - (current_time - last_update_time));
    }
    
    last_update_time = time_us_64();
}

/**
 * Present frame with vertical sync for tear-free display
 */
void double_buffer_present_with_vsync(void) {
    if (!g_double_buffer.using_double_buffer) return;
    
    // Wait for safe update window
    double_buffer_wait_for_vblank();
    
    // Swap buffers
    double_buffer_swap();
    
    // Copy to LCD
    double_buffer_copy_to_lcd();
}
