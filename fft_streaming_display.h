/**
 * fft_streaming_display.h - Fixed Scale FFT Streaming Display
 * 
 * Features:
 * - Fixed axis scales: 100Hz-50kHz (logarithmic), -100dBm to +20dBm (linear)
 * - Bright white axis labels for high visibility
 * - Anti-flashing optimized streaming display
 * - Configurable update rate and display dimensions (320x240 landscape)
 */

#ifndef FFT_STREAMING_DISPLAY_H
#define FFT_STREAMING_DISPLAY_H

#include "LCD_Driver.h"
#include "LCD_GUI.h"
#include <stdbool.h>

// Display configuration constants for 320x240 landscape display
#define STREAM_SPECTRUM_X 40        // X position with space for Y-axis labels
#define STREAM_SPECTRUM_Y 20        // Y position with space for title
#define STREAM_SPECTRUM_W 240       // Width (320-40-40 for margins)
#define STREAM_SPECTRUM_H 180       // Height (240-20-40 for X-axis labels)

#define STREAM_FFT_SIZE 1024        // FFT size (must match analyzer - CORRECTED from 512 to 1024)
#define STREAM_BUFFER_COLS 240      // Buffer columns (matches spectrum width)
#define STREAM_UPDATE_WIDTH 4       // Pixels to update per frame

// Fixed axis scale constants - main.cで定義された値を使用
#define STREAM_FREQ_MIN_HZ 1000     // Minimum frequency (1kHz) - main.cで設定
#define STREAM_FREQ_MAX_HZ 50000    // Maximum frequency (50kHz) - main.cで設定

// Frequency axis scale configuration - main.cで定義された設定を使用
// 設定は main.c の USE_LOG_FREQ_SCALE 定数で制御されます
#define STREAM_AMP_MIN_DBM -100     // Minimum amplitude (-100dBm)
#define STREAM_AMP_MAX_DBM 20       // Maximum amplitude (+20dBm)

// Color definitions for high visibility
#define STREAM_COLOR_BG 0x0000       // Black background
#define STREAM_COLOR_SPECTRUM 0x07E0  // Green spectrum
#define STREAM_COLOR_GRID 0x3186      // Dark gray grid
#define STREAM_COLOR_AXIS 0xFFFF      // White axis labels (bright)

// Point structure for spectrum buffer (renamed to avoid conflicts)
typedef struct {
    int x, y;
} SpectrumPoint;

// Hold buffer structure for 2 second peak hold functionality
typedef struct {
    float peak_db;           // Peak dB value (stored in dB)
    absolute_time_t hold_time; // Time when this peak was set (absolute time)
} SpectrumHold;

// Display configuration for hold functionality - main.cで定義された設定を使用
// ピークホールド時間は main.c の PEAK_HOLD_DURATION_MS 定数で制御されます
#define STREAM_HOLD_COLOR 0x07FF      // Cyan color for held peaks

// Display statistics structure
typedef struct {
    int buffer_cols;
    int update_width;
    int spectrum_area_x, spectrum_area_y;
    int spectrum_area_w, spectrum_area_h;
    int frequency_range_hz_min, frequency_range_hz_max;
    int amplitude_range_dbm_min, amplitude_range_dbm_max;
} fft_streaming_display_stats_t;

// Core display functions
void fft_streaming_display_init(void);
void fft_streaming_display_clear(void);
void fft_streaming_display_update_spectrum(const float* magnitude_db, float sample_rate);
void fft_streaming_display_render_buffer(void);
void fft_streaming_display_get_stats(fft_streaming_display_stats_t* stats);

// Frequency scaling functions
float fft_streaming_display_freq_to_position(float freq_hz);
int fft_streaming_display_freq_to_column(float freq_hz);

// Axis and grid drawing functions
void fft_streaming_display_draw_axes(void);
void fft_streaming_display_draw_grid(void);

// Test function for axis display only
void fft_streaming_display_test_axes_only(void);

#endif // FFT_STREAMING_DISPLAY_H
