/*****************************************************************************
* | File      	:   fft_smooth_display.c
* | Author      :   
* | Function    :   Ultra-smooth FFT display with partial updates
* | Info        :   
*   - Partial update optimization
*   - Line-based spectrum updates
*   - Minimal LCD transfer overhead
*----------------
******************************************************************************/

#include "fft_smooth_display.h"
#include "lcd_partial_update.h"
#include "fft_analyzer.h"
#include "LCD_GUI.h"
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

// Display configuration
#define DISPLAY_WIDTH   480
#define DISPLAY_HEIGHT  320
#define SPECTRUM_X      40
#define SPECTRUM_Y      40
#define SPECTRUM_W      400
#define SPECTRUM_H      200
#define MAX_SPECTRUM_POINTS 200

// Color definitions (using unique names to avoid conflicts)
#define SMOOTH_COLOR_BG        BLACK
#define SMOOTH_COLOR_GRID      0x39E7    // Dark gray
#define SMOOTH_COLOR_SPECTRUM  GREEN
#define SMOOTH_COLOR_PEAK      RED
#define SMOOTH_COLOR_TEXT      WHITE

// Previous spectrum data for differential updates
static POINT prev_spectrum_y[MAX_SPECTRUM_POINTS];
static bool prev_data_valid = false;
static int spectrum_point_count = 0;

/**
 * Initialize smooth FFT display
 */
bool fft_smooth_display_init(void) {
    // Initialize partial update system
    if (!partial_update_init()) {
        return false;
    }
    
    // Clear screen and draw static elements once
    fft_smooth_display_draw_background();
    partial_update_flush();
    
    prev_data_valid = false;
    spectrum_point_count = 0;
    
    return true;
}

/**
 * Cleanup smooth display
 */
void fft_smooth_display_cleanup(void) {
    partial_update_cleanup();
}

/**
 * Draw background and static elements (called once)
 */
void fft_smooth_display_draw_background(void) {
    // Fill entire screen with background color
    for (int y = 0; y < DISPLAY_HEIGHT; y++) {
        for (int x = 0; x < DISPLAY_WIDTH; x++) {
            partial_update_set_pixel(x, y, SMOOTH_COLOR_BG);
        }
    }
    
    // Draw spectrum area outline
    for (int x = SPECTRUM_X - 1; x <= SPECTRUM_X + SPECTRUM_W; x++) {
        partial_update_set_pixel(x, SPECTRUM_Y - 1, SMOOTH_COLOR_GRID);
        partial_update_set_pixel(x, SPECTRUM_Y + SPECTRUM_H, SMOOTH_COLOR_GRID);
    }
    for (int y = SPECTRUM_Y - 1; y <= SPECTRUM_Y + SPECTRUM_H; y++) {
        partial_update_set_pixel(SPECTRUM_X - 1, y, SMOOTH_COLOR_GRID);
        partial_update_set_pixel(SPECTRUM_X + SPECTRUM_W, y, SMOOTH_COLOR_GRID);
    }
    
    // Draw frequency grid (simplified)
    for (int i = 1; i < 5; i++) {
        int x = SPECTRUM_X + (i * SPECTRUM_W / 5);
        for (int y = SPECTRUM_Y; y <= SPECTRUM_Y + SPECTRUM_H; y += 8) {
            partial_update_set_pixel(x, y, SMOOTH_COLOR_GRID);
        }
    }
    
    // Draw amplitude grid
    for (int i = 1; i < 4; i++) {
        int y = SPECTRUM_Y + (i * SPECTRUM_H / 4);
        for (int x = SPECTRUM_X; x <= SPECTRUM_X + SPECTRUM_W; x += 8) {
            partial_update_set_pixel(x, y, SMOOTH_COLOR_GRID);
        }
    }
}

/**
 * Update spectrum with minimal LCD transfers
 */
void fft_smooth_display_update_spectrum(const float* magnitude_db) {
    if (!g_partial_update.enabled) return;
    
    // Calculate new spectrum points
    POINT new_spectrum_y[MAX_SPECTRUM_POINTS];
    int new_point_count = 0;
    
    // Sample spectrum data at regular X intervals
    for (int i = 0; i < MAX_SPECTRUM_POINTS && i < SPECTRUM_W; i++) {
        int x = SPECTRUM_X + i * 2;  // Every 2 pixels
        
        // Map X coordinate to frequency bin
        float x_ratio = (float)i / (float)MAX_SPECTRUM_POINTS;
        int bin = (int)(x_ratio * (FFT_SIZE / 2));
        if (bin >= FFT_SIZE / 2) bin = FFT_SIZE / 2 - 1;
        
        // Get magnitude and convert to Y coordinate
        float magnitude = magnitude_db[bin];
        float normalized_mag = (magnitude + 80.0f) / 80.0f;  // Normalize -80dB to 0dB
        if (normalized_mag < 0.0f) normalized_mag = 0.0f;
        if (normalized_mag > 1.0f) normalized_mag = 1.0f;
        
        POINT y = SPECTRUM_Y + SPECTRUM_H - (POINT)(normalized_mag * SPECTRUM_H);
        new_spectrum_y[new_point_count] = y;
        new_point_count++;
    }
    
    // Erase previous spectrum (only changed areas)
    if (prev_data_valid) {
        for (int i = 0; i < spectrum_point_count && i < MAX_SPECTRUM_POINTS; i++) {
            int x = SPECTRUM_X + i * 2;
            
            // Erase vertical line from previous Y to bottom
            POINT old_y = prev_spectrum_y[i];
            for (int y = old_y; y < SPECTRUM_Y + SPECTRUM_H; y++) {
                partial_update_set_pixel(x, y, SMOOTH_COLOR_BG);
            }
        }
    }
    
    // Draw new spectrum
    for (int i = 0; i < new_point_count; i++) {
        int x = SPECTRUM_X + i * 2;
        POINT y = new_spectrum_y[i];
        
        // Draw vertical line from Y to bottom
        for (int py = y; py < SPECTRUM_Y + SPECTRUM_H; py++) {
            partial_update_set_pixel(x, py, SMOOTH_COLOR_SPECTRUM);
        }
    }
    
    // Store current data for next frame
    memcpy(prev_spectrum_y, new_spectrum_y, sizeof(POINT) * new_point_count);
    spectrum_point_count = new_point_count;
    prev_data_valid = true;
}

/**
 * Show performance info (minimal)
 */
void fft_smooth_display_show_info(float fps) {
    // Simple FPS indicator (bar length represents FPS)
    int fps_bar_length = (int)(fps / 2);  // Max 60 pixels for 120 FPS
    if (fps_bar_length > 60) fps_bar_length = 60;
    
    // Clear previous indicator
    for (int x = 10; x < 80; x++) {
        for (int y = 10; y < 15; y++) {
            partial_update_set_pixel(x, y, SMOOTH_COLOR_BG);
        }
    }
    
    // Draw FPS bar
    COLOR fps_color = (fps > 30) ? GREEN : (fps > 15) ? 0xFFE0 : RED;
    for (int x = 10; x < 10 + fps_bar_length; x++) {
        for (int y = 10; y < 15; y++) {
            partial_update_set_pixel(x, y, fps_color);
        }
    }
}

/**
 * Present frame (flush partial updates)
 */
void fft_smooth_display_present(void) {
    partial_update_flush();
}
