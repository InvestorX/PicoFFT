/*****************************************************************************
* | File      	:   fft_display.c
* | Author      :   
* | Function    :   FFT spectrum display functions
* | Info        :   Display spectrum analysis results on LCD
*----------------
******************************************************************************/

#include "fft_analyzer.h"

/**
 * Display FFT spectrum on LCD
 */
void fft_display_spectrum(LCD_SCAN_DIR scan_dir) {
    // Clear spectrum area
    LCD_SetArealColor(SPECTRUM_X_OFFSET, SPECTRUM_Y_OFFSET, 
                      SPECTRUM_X_OFFSET + SPECTRUM_WIDTH, 
                      SPECTRUM_Y_OFFSET + SPECTRUM_HEIGHT, 
                      COLOR_BACKGROUND);
    
    // Draw grid
    fft_draw_grid();
    
    // Draw frequency and amplitude labels
    fft_draw_frequency_labels();
    fft_draw_amplitude_labels();
    
    // Draw spectrum as connected line graph
    int prev_x = SPECTRUM_X_OFFSET;
    int prev_y = SPECTRUM_Y_OFFSET + SPECTRUM_HEIGHT;
    
    for (int i = 0; i < FREQ_BINS; i++) {
        // Calculate frequency for this bin (logarithmic scale)
        float freq = MIN_FREQ * pow(10.0, (float)i / FREQ_BINS * log10((float)MAX_FREQ / MIN_FREQ));
        
        // Convert frequency to X coordinate
        int x = fft_frequency_to_display_x(freq);
        
        // Convert magnitude to Y coordinate
        int y = fft_magnitude_to_display_y(g_fft_analyzer.spectrum_data[i]);
        
        // Draw line from previous point to current point
        if (i > 0) {
            GUI_DrawLine(prev_x, prev_y, x, y, COLOR_SPECTRUM, LINE_SOLID, DOT_PIXEL_1X1);
        }
        
        // Draw a small dot at the data point for visibility
        GUI_DrawPoint(x, y, COLOR_SPECTRUM, DOT_PIXEL_1X1, DOT_FILL_AROUND);
        
        prev_x = x;
        prev_y = y;
    }
    
    // Draw title
    GUI_DisString_EN(SPECTRUM_X_OFFSET, 10, "FFT Spectrum Analyzer (100Hz - 50kHz)", &Font16, COLOR_BACKGROUND, COLOR_TEXT);
    
    // Draw current sampling info
    char info_str[64];
    sprintf(info_str, "Sampling: %d kHz, FFT Size: %d, DMA Mode", SAMPLE_RATE/1000, FFT_SIZE);
    GUI_DisString_EN(SPECTRUM_X_OFFSET, SPECTRUM_Y_OFFSET + SPECTRUM_HEIGHT + 10, info_str, &Font12, COLOR_BACKGROUND, COLOR_TEXT);
    
    // Show real-time update indicator
    static int update_counter = 0;
    update_counter++;
    char update_str[32];
    sprintf(update_str, "Updates: %d", update_counter);
    GUI_DisString_EN(SPECTRUM_X_OFFSET + 300, SPECTRUM_Y_OFFSET + SPECTRUM_HEIGHT + 10, update_str, &Font12, COLOR_BACKGROUND, COLOR_TEXT);
}

/**
 * Draw grid lines
 */
void fft_draw_grid(void) {
    // Vertical grid lines (frequency markers)
    float freq_markers[] = {100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000};
    int num_markers = sizeof(freq_markers) / sizeof(freq_markers[0]);
    
    for (int i = 0; i < num_markers; i++) {
        if (freq_markers[i] >= MIN_FREQ && freq_markers[i] <= MAX_FREQ) {
            int x = fft_frequency_to_display_x(freq_markers[i]);
            GUI_DrawLine(x, SPECTRUM_Y_OFFSET, x, SPECTRUM_Y_OFFSET + SPECTRUM_HEIGHT, COLOR_GRID, LINE_SOLID, DOT_PIXEL_1X1);
        }
    }
    
    // Horizontal grid lines (amplitude markers)
    for (int i = 0; i <= 4; i++) {
        int y = SPECTRUM_Y_OFFSET + (SPECTRUM_HEIGHT * i / 4);
        GUI_DrawLine(SPECTRUM_X_OFFSET, y, SPECTRUM_X_OFFSET + SPECTRUM_WIDTH, y, COLOR_GRID, LINE_SOLID, DOT_PIXEL_1X1);
    }
    
    // Draw border
    GUI_DrawRectangle(SPECTRUM_X_OFFSET, SPECTRUM_Y_OFFSET, 
                      SPECTRUM_X_OFFSET + SPECTRUM_WIDTH, 
                      SPECTRUM_Y_OFFSET + SPECTRUM_HEIGHT, 
                      COLOR_TEXT, DRAW_EMPTY, DOT_PIXEL_1X1);
}

/**
 * Draw frequency labels
 */
void fft_draw_frequency_labels(void) {
    float freq_labels[] = {100, 1000, 10000, 50000};
    char* freq_texts[] = {"100Hz", "1kHz", "10kHz", "50kHz"};
    int num_labels = sizeof(freq_labels) / sizeof(freq_labels[0]);
    
    for (int i = 0; i < num_labels; i++) {
        if (freq_labels[i] >= MIN_FREQ && freq_labels[i] <= MAX_FREQ) {
            int x = fft_frequency_to_display_x(freq_labels[i]);
            GUI_DisString_EN(x - 20, SPECTRUM_Y_OFFSET + SPECTRUM_HEIGHT + 25, 
                           freq_texts[i], &Font12, COLOR_BACKGROUND, COLOR_TEXT);
        }
    }
}

/**
 * Draw amplitude labels
 */
void fft_draw_amplitude_labels(void) {
    char* amp_labels[] = {"0dB", "-25dB", "-50dB", "-75dB", "-100dB"};
    
    for (int i = 0; i < 5; i++) {
        int y = SPECTRUM_Y_OFFSET + (SPECTRUM_HEIGHT * i / 4) - 6;
        GUI_DisString_EN(SPECTRUM_X_OFFSET - 50, y, amp_labels[i], &Font12, COLOR_BACKGROUND, COLOR_TEXT);
    }
}
