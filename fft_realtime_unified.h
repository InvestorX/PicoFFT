/*****************************************************************************
* | File      	:   fft_realtime_unified.h
* | Author      :   PicoFFT Project
* | Function    :   Header for unified real-time FFT analysis
* | Info        :   
*   - Unified interface for real-time FFT analysis
*   - Supports both manual and DMA-based ADC sampling
*   - Configurable via config_settings.h
*   - Performance monitoring and error handling
*----------------
******************************************************************************/

#ifndef __FFT_REALTIME_UNIFIED_H
#define __FFT_REALTIME_UNIFIED_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"

// Display configuration (should match streaming display system)
#define STREAM_BUFFER_COLS 240  // LCD width for spectrum columns

// ========================================
// 🔧 Unified Real-time FFT API
// ========================================

/**
 * Initialize unified real-time FFT analysis system
 * This initializes:
 * - System hardware (System_Init)
 * - LCD display in landscape mode
 * - Streaming display system
 * - ADC sampling system (DMA or Manual based on config)
 * - Performance monitoring
 * 
 * @return true if initialization successful, false on error
 */
bool fft_realtime_unified_init(void);

/**
 * Main unified real-time FFT analysis loop
 * This is the main processing loop that:
 * - Checks for new ADC data
 * - Processes FFT when data available
 * - Updates display with spectrum data
 * - Monitors performance and errors
 * - Controls frame rate
 * 
 * Note: This function runs indefinitely until system reset
 */
void fft_realtime_unified_run(void);

/**
 * Update display with new spectrum data
 * Converts FFT magnitude spectrum to display format and updates
 * the streaming display system
 * 
 * @param magnitude_spectrum Pointer to FFT magnitude array (512 elements)
 */
void fft_realtime_unified_update_display(float* magnitude_spectrum);

/**
 * Debug function: Print frequency mapping details
 * Outputs detailed information about frequency-to-display mapping
 * for debugging spectrum display issues
 * 
 * @param magnitude_spectrum Pointer to FFT magnitude array for analysis
 */
void fft_realtime_unified_debug_frequency_mapping(float* magnitude_spectrum);

/**
 * Debug function: Print Y-axis amplitude mapping details
 * Outputs detailed information about dBm-to-Y-coordinate mapping
 * for debugging amplitude display issues
 * 
 * @param magnitude_spectrum Pointer to FFT magnitude array for analysis
 */
void fft_realtime_unified_debug_amplitude_mapping(float* magnitude_spectrum);

/**
 * Print comprehensive system status information
 * Displays:
 * - Performance metrics (FPS, errors)
 * - ADC sampling status (rate, overruns)
 * - Configuration details (window, ranges)
 */
void fft_realtime_unified_print_status(void);

/**
 * Get current window function name as string
 * @return String name of current window function
 */
const char* fft_realtime_unified_get_window_name(void);

/**
 * Get current window function amplitude correction factor
 * @return Amplitude correction factor for current window
 */
float fft_realtime_unified_get_window_correction(void);

/**
 * Cleanup and shutdown unified system
 * Stops sampling, prints final statistics, and cleans up resources
 */
void fft_realtime_unified_cleanup(void);

// ========================================
// 🔧 Performance Monitoring Functions
// ========================================

/**
 * Get current actual frame rate
 * @return Current FPS measurement
 */
float fft_realtime_unified_get_actual_fps(void);

/**
 * Get total frame count since start
 * @return Total frames processed
 */
uint32_t fft_realtime_unified_get_frame_count(void);

/**
 * Get total error count since start
 * @return Total processing errors encountered
 */
uint32_t fft_realtime_unified_get_error_count(void);

/**
 * Reset performance counters
 */
void fft_realtime_unified_reset_counters(void);

#endif // __FFT_REALTIME_UNIFIED_H
