/*****************************************************************************
* | File      	:   lcd_partial_update.c
* | Author      :   
* | Function    :   Partial LCD update implementation
* | Info        :   
*   - Updates only changed regions
*   - Dramatically reduces transfer time
*   - Enables smooth animation on LCD
*----------------
******************************************************************************/

#include "lcd_partial_update.h"
#include "LCD_Driver.h"
#include "DEV_Config.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

// Global partial update instance
partial_update_t g_partial_update = {
    .region_count = 0,
    .enabled = false
};

// Screen buffer for tracking changes
static uint16_t* screen_buffer = NULL;
static uint16_t* temp_buffer = NULL;

/**
 * Initialize partial update system
 */
bool partial_update_init(void) {
    // Allocate screen buffer to track current state
    screen_buffer = (uint16_t*)malloc(LCD_X_MAXPIXEL * LCD_Y_MAXPIXEL * sizeof(uint16_t));
    temp_buffer = (uint16_t*)malloc(LCD_X_MAXPIXEL * sizeof(uint16_t));
    
    if (!screen_buffer || !temp_buffer) {
        partial_update_cleanup();
        return false;
    }
    
    // Initialize with black screen
    memset(screen_buffer, 0, LCD_X_MAXPIXEL * LCD_Y_MAXPIXEL * sizeof(uint16_t));
    
    g_partial_update.region_count = 0;
    g_partial_update.enabled = true;
    
    return true;
}

/**
 * Cleanup partial update system
 */
void partial_update_cleanup(void) {
    if (screen_buffer) {
        free(screen_buffer);
        screen_buffer = NULL;
    }
    if (temp_buffer) {
        free(temp_buffer);
        temp_buffer = NULL;
    }
    g_partial_update.enabled = false;
}

/**
 * Mark a region as dirty (needs update)
 */
void partial_update_mark_dirty(POINT x1, POINT y1, POINT x2, POINT y2) {
    if (!g_partial_update.enabled || g_partial_update.region_count >= MAX_UPDATE_REGIONS) return;
    
    // Clamp to screen bounds
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= LCD_X_MAXPIXEL) x2 = LCD_X_MAXPIXEL - 1;
    if (y2 >= LCD_Y_MAXPIXEL) y2 = LCD_Y_MAXPIXEL - 1;
    
    if (x1 > x2 || y1 > y2) return;
    
    // Try to merge with existing regions
    for (int i = 0; i < g_partial_update.region_count; i++) {
        update_region_t* region = &g_partial_update.regions[i];
        
        // Check if regions overlap or are adjacent
        if (x1 <= region->x2 + 1 && x2 >= region->x1 - 1 &&
            y1 <= region->y2 + 1 && y2 >= region->y1 - 1) {
            // Merge regions
            region->x1 = (x1 < region->x1) ? x1 : region->x1;
            region->y1 = (y1 < region->y1) ? y1 : region->y1;
            region->x2 = (x2 > region->x2) ? x2 : region->x2;
            region->y2 = (y2 > region->y2) ? y2 : region->y2;
            region->dirty = true;
            return;
        }
    }
    
    // Add new region
    update_region_t* region = &g_partial_update.regions[g_partial_update.region_count];
    region->x1 = x1;
    region->y1 = y1;
    region->x2 = x2;
    region->y2 = y2;
    region->dirty = true;
    g_partial_update.region_count++;
}

/**
 * Clear all dirty regions
 */
void partial_update_clear_regions(void) {
    g_partial_update.region_count = 0;
}

/**
 * Flush all dirty regions to LCD
 */
void partial_update_flush(void) {
    if (!g_partial_update.enabled || !screen_buffer) return;
    
    for (int i = 0; i < g_partial_update.region_count; i++) {
        update_region_t* region = &g_partial_update.regions[i];
        if (!region->dirty) continue;
        
        // Set LCD window to this region
        LCD_SetWindow(region->x1, region->y1, region->x2, region->y2);
        
        // Optimized line-by-line transfer
        DEV_Digital_Write(LCD_CS_PIN, 0);
        DEV_Digital_Write(LCD_DC_PIN, 1);
        
        for (POINT y = region->y1; y <= region->y2; y++) {
            for (POINT x = region->x1; x <= region->x2; x++) {
                uint16_t color = screen_buffer[y * LCD_X_MAXPIXEL + x];
                SPI4W_Write_Byte(color >> 8);
                SPI4W_Write_Byte(color & 0xFF);
            }
        }
        
        DEV_Digital_Write(LCD_CS_PIN, 1);
        region->dirty = false;
    }
    
    partial_update_clear_regions();
}

/**
 * Set pixel with automatic dirty region tracking
 */
void partial_update_set_pixel(POINT x, POINT y, COLOR color) {
    if (!g_partial_update.enabled || !screen_buffer) return;
    if (x >= LCD_X_MAXPIXEL || y >= LCD_Y_MAXPIXEL) return;
    
    uint16_t* pixel = &screen_buffer[y * LCD_X_MAXPIXEL + x];
    if (*pixel != color) {
        *pixel = color;
        partial_update_mark_dirty(x, y, x, y);
    }
}

/**
 * Draw line with automatic dirty region tracking
 */
void partial_update_draw_line(POINT x1, POINT y1, POINT x2, POINT y2, COLOR color) {
    if (!g_partial_update.enabled) return;
    
    // Bresenham's line algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;
    
    POINT x = x1, y = y1;
    POINT min_x = x1, max_x = x1, min_y = y1, max_y = y1;
    
    while (true) {
        partial_update_set_pixel(x, y, color);
        
        // Track bounding box
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
        
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
    
    // Mark the bounding box as dirty
    partial_update_mark_dirty(min_x, min_y, max_x, max_y);
}
