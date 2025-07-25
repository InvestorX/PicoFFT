/*****************************************************************************
* | File      	:   fft_realtime_unified.c
* | Author      :   PicoFFT Project
* | Function    :   Unified real-time FFT analysis with configurable ADC sampling
* | Info        :   
*   - Supports both manual and DMA-based ADC sampling
*   - Configurable via config_settings.h (ADC_DMA_ENABLED)
*   - Integrated with existing display system
*   - Performance monitoring and error handling
*----------------
******************************************************************************/

#include "fft_realtime_unified.h"
#include "adc_sampling.h"
#include "fft_streaming_display.h"
#include "config_settings.h"
#include "DEV_Config.h"
#include "LCD_Driver.h"
#include <stdio.h>
#include <math.h>

// Performance monitoring variables
static absolute_time_t last_frame_time;
static float actual_fps = 0.0f;
static uint32_t frame_count = 0;
static uint32_t error_count = 0;

/**
 * Initialize unified real-time FFT analysis system
 */
bool fft_realtime_unified_init(void) {
    printf("=== Initializing Unified Real-time FFT Analysis System ===\n");
    
    // Initialize system hardware
    printf("Initializing system...\n");
    System_Init();
    
    // Initialize LCD for landscape mode
    printf("Initializing LCD for landscape mode...\n");
    LCD_Init(D2U_L2R, 100);  // Down to up, left to right, 100% backlight
    LCD_Clear(BLACK);
    printf("LCD initialized.\n");
    
    // Initialize streaming display system
    printf("Initializing streaming display system...\n");
    fft_streaming_display_init();
    printf("Streaming display system initialized.\n");
    
    // Initialize unified ADC sampling system
    adc_sampling_mode_t mode = ADC_DMA_ENABLED ? ADC_MODE_DMA : ADC_MODE_MANUAL;
    printf("Initializing ADC sampling system in %s mode...\n", 
           mode == ADC_MODE_DMA ? "DMA" : "Manual");
    
    if (!adc_sampling_init(mode)) {
        printf("ERROR: Failed to initialize ADC sampling system!\n");
        return false;
    }
    
    // Start ADC sampling
    if (!adc_sampling_start()) {
        printf("ERROR: Failed to start ADC sampling!\n");
        return false;
    }
    
    // Initialize performance monitoring
    last_frame_time = get_absolute_time();
    frame_count = 0;
    error_count = 0;
    actual_fps = 0.0f;
    
    printf("=== Unified Real-time FFT Analysis System Initialized ===\n");
    printf("Configuration:\n");
    printf("  ADC Mode: %s\n", mode == ADC_MODE_DMA ? "DMA" : "Manual");
    printf("  Sampling Rate: %d Hz\n", SAMPLING_RATE_HZ);
    printf("  FFT Size: %d\n", 1024);
    printf("  Target FPS: %d\n", TARGET_FPS);
    printf("  Window Function: %s (Type=%d)\n", 
           fft_realtime_unified_get_window_name(), FFT_WINDOW_TYPE);
    printf("  Frequency Range: %d - %d Hz\n", FREQUENCY_RANGE_MIN, FREQUENCY_RANGE_MAX);
    printf("  Amplitude Range: %d to %d dBm\n", AMPLITUDE_RANGE_MIN_DB, AMPLITUDE_RANGE_MAX_DB);
    
    return true;
}

/**
 * Main unified real-time FFT analysis loop
 */
void fft_realtime_unified_run(void) {
    printf("Starting unified real-time FFT analysis loop...\n");
    
    // Main processing loop
    while (true) {
        absolute_time_t frame_start = get_absolute_time();
        
        // Check if new ADC data is available
        if (adc_sampling_is_ready()) {
            
            // Process FFT on current buffer
            if (adc_sampling_process_fft()) {
                
                // Get FFT magnitude spectrum
                float* magnitude = adc_sampling_get_magnitude_spectrum();
                if (magnitude != NULL) {
                    
                    // Update streaming display with new spectrum data
                    fft_realtime_unified_update_display(magnitude);
                    
                    // Update performance counters
                    frame_count++;
                    
                    // Print periodic status (every 100 frames)
                    if (frame_count % 100 == 0) {
                        fft_realtime_unified_print_status();
                    }
                }
                
                // Signal that processing is complete
                adc_sampling_complete_processing();
                
            } else {
                error_count++;
                printf("Warning: FFT processing failed (error #%lu)\n", error_count);
            }
        }
        
        // Calculate frame timing
        absolute_time_t frame_end = get_absolute_time();
        int64_t frame_time_us = absolute_time_diff_us(frame_start, frame_end);
        
        // Update FPS calculation
        int64_t total_frame_time_us = absolute_time_diff_us(last_frame_time, frame_end);
        if (total_frame_time_us > 0) {
            actual_fps = 1000000.0f / (float)total_frame_time_us;
        }
        last_frame_time = frame_end;
        
        // Frame rate control
        int64_t target_frame_time_us = TARGET_FRAME_TIME_US;
        if (frame_time_us < target_frame_time_us) {
            int64_t sleep_time_us = target_frame_time_us - frame_time_us;
            if (sleep_time_us > 0) {
                sleep_us(sleep_time_us);
            }
        }
        
        // Check for errors and warnings
        if (adc_sampling_get_overrun_count() > 0) {
            if (frame_count % 1000 == 0) {  // Print warning every 1000 frames
                printf("Warning: %lu buffer overruns detected\n", 
                       adc_sampling_get_overrun_count());
            }
        }
    }
}

/**
 * Debug function: Print Y-axis amplitude mapping details
 */
void fft_realtime_unified_debug_amplitude_mapping(float* magnitude_spectrum) {
    printf("\n=== 📊 Y-Axis Amplitude Mapping Debug ===\n");
    printf("Amplitude Range: %d to %d dBm\n", AMPLITUDE_RANGE_MIN_DB, AMPLITUDE_RANGE_MAX_DB);
    printf("Display Height: %d pixels\n", STREAM_SPECTRUM_H);
    printf("Window Correction: %.3f (%.1f dB)\n", 
           fft_realtime_unified_get_window_correction(),
           20.0f * log10f(fft_realtime_unified_get_window_correction()));
    
    printf("\n🎯 20kHz Signal Analysis:\n");
    printf("Raw_dB | Corrected_dB | Normalized | Y_Coord | From_Bottom | Expected_Y\n");
    printf("-------|--------------|------------|---------|-------------|----------\n");
    
    // Target 20kHz analysis
    int fft_bin_20k = (int)(20000.0f * ADC_SAMPLING_FFT_SIZE / ADC_SAMPLING_RATE + 0.5f);
    float raw_db = magnitude_spectrum[fft_bin_20k];
    float window_correction = fft_realtime_unified_get_window_correction();
    float corrected_db = raw_db + 20.0f * log10f(window_correction);
    
    // Calculate normalized position and Y coordinate (same as display system)
    float db_range = (float)(AMPLITUDE_RANGE_MAX_DB - AMPLITUDE_RANGE_MIN_DB);  // 120dB range
    float normalized = (corrected_db - AMPLITUDE_RANGE_MIN_DB) / db_range;
    if (normalized < 0.0f) normalized = 0.0f;
    if (normalized > 1.0f) normalized = 1.0f;
    
    int height = (int)(normalized * STREAM_SPECTRUM_H);
    if (height < 0) height = 0;
    if (height >= STREAM_SPECTRUM_H) height = STREAM_SPECTRUM_H - 1;
    
    int y_coord = STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H - height;  // From bottom
    int from_bottom = STREAM_SPECTRUM_H - height;
    
    // Expected Y for 0dBm signal (should be 120/120 * 180 = 150 pixels from bottom)
    float expected_0dbm_normalized = (0.0f - AMPLITUDE_RANGE_MIN_DB) / db_range;  // (0-(-100))/120 = 0.833
    int expected_0dbm_height = (int)(expected_0dbm_normalized * STREAM_SPECTRUM_H);  // 0.833 * 180 = 150
    int expected_0dbm_y = STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H - expected_0dbm_height;  // 20 + 180 - 150 = 50
    
    printf("%6.1f | %12.1f | %10.3f | %7d | %11d | %8d\n",
           raw_db, corrected_db, normalized, y_coord, from_bottom, expected_0dbm_y);
    
    printf("\n📏 dBm Scale Reference Points:\n");
    printf("dBm   | Normalized | Height | Y_Coord | From_Bottom\n");
    printf("------|------------|--------|---------|----------\n");
    
    int reference_dbm[] = {20, 10, 0, -10, -20, -40, -60, -80, -100};
    int ref_count = sizeof(reference_dbm) / sizeof(reference_dbm[0]);
    
    for (int i = 0; i < ref_count; i++) {
        float ref_normalized = (reference_dbm[i] - AMPLITUDE_RANGE_MIN_DB) / db_range;
        int ref_height = (int)(ref_normalized * STREAM_SPECTRUM_H);
        if (ref_height < 0) ref_height = 0;
        if (ref_height >= STREAM_SPECTRUM_H) ref_height = STREAM_SPECTRUM_H - 1;
        int ref_y = STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H - ref_height;
        int ref_from_bottom = STREAM_SPECTRUM_H - ref_height;
        
        printf("%5d | %10.3f | %6d | %7d | %11d\n",
               reference_dbm[i], ref_normalized, ref_height, ref_y, ref_from_bottom);
    }
    
    printf("\n🔬 ADC Input Analysis:\n");
    printf("Signal Level | ADC Input (V) | ADC Digital | FFT Magnitude | Raw dB\n");
    printf("-------------|---------------|-------------|---------------|--------\n");
    
    // Analyze signal levels
    float signal_levels[] = {1.0f, 0.5f, 0.274f, 0.1f, 0.05f};  // Voltage levels
    int level_count = sizeof(signal_levels) / sizeof(signal_levels[0]);
    
    for (int i = 0; i < level_count; i++) {
        float voltage = signal_levels[i];
        float adc_input = voltage + ADC_OFFSET_VOLTAGE;  // Add DC offset
        int adc_digital = (int)(adc_input / ADC_REFERENCE_VOLTAGE * 4096);
        
        // Estimate FFT magnitude (simplified calculation)
        float fft_magnitude = voltage * 1024 / 2;  // Rough estimation
        float estimated_db = 20.0f * log10f(fft_magnitude / (0.274f * 1024 / 2));  // Relative to 0dBm
        
        printf("%8.3f V   | %9.3f V   | %9d   | %11.0f     | %6.1f\n",
               voltage, adc_input, adc_digital, fft_magnitude, estimated_db);
    }
    
    printf("===============================================\n\n");
}

/**
 * Debug function: Print frequency mapping details
 */
void fft_realtime_unified_debug_frequency_mapping(float* magnitude_spectrum) {
    printf("\n=== 🔍 Frequency Mapping Debug ===\n");
    printf("FFT_SIZE: %d, SAMPLE_RATE: %d Hz\n", ADC_SAMPLING_FFT_SIZE, ADC_SAMPLING_RATE);
    printf("STREAM_BUFFER_COLS: %d\n", STREAM_BUFFER_COLS);
    
    int fft_bins_per_col = (ADC_SAMPLING_FFT_SIZE/2) / STREAM_BUFFER_COLS;
    if (fft_bins_per_col < 1) fft_bins_per_col = 1;
    printf("FFT bins per column: %d\n", fft_bins_per_col);
    
    printf("\n📊 22.5kHz Signal Analysis vs Axis Labels:\n");
    printf("Test_Freq | FFT_Bin | Bin_Freq | Axis_X | Spectrum_Col | Spectrum_X | X_Diff\n");
    printf("----------|---------|----------|--------|--------------|------------|-------\n");
    
    // Test frequencies around 22.5kHz
    float test_freqs[] = {20000, 22500, 25000, 27500, 30000};
    int test_count = sizeof(test_freqs) / sizeof(test_freqs[0]);
    
    for (int i = 0; i < test_count; i++) {
        float test_freq = test_freqs[i];
        
        // Calculate FFT bin for this frequency
        int fft_bin = (int)(test_freq * ADC_SAMPLING_FFT_SIZE / ADC_SAMPLING_RATE + 0.5f);
        float actual_bin_freq = (float)fft_bin * ADC_SAMPLING_RATE / ADC_SAMPLING_FFT_SIZE;
        
        // Calculate axis label position (using display system's function)
        float normalized_axis = (test_freq - FREQUENCY_RANGE_MIN) / (FREQUENCY_RANGE_MAX - FREQUENCY_RANGE_MIN);
        int axis_x = STREAM_SPECTRUM_X + (int)(normalized_axis * STREAM_SPECTRUM_W);
        
        // Calculate spectrum display position (using display system's bin processing)
        // This simulates what fft_streaming_display_update_spectrum() does
        int spectrum_col = -1;
        int spectrum_x = -1;
        
        // Simulate display system's bin-to-column mapping
        float bin_freq_for_display = actual_bin_freq;
        if (bin_freq_for_display >= FREQUENCY_RANGE_MIN && bin_freq_for_display <= FREQUENCY_RANGE_MAX) {
            float normalized_spectrum = (bin_freq_for_display - FREQUENCY_RANGE_MIN) / (FREQUENCY_RANGE_MAX - FREQUENCY_RANGE_MIN);
            spectrum_col = (int)(normalized_spectrum * STREAM_BUFFER_COLS);
            if (spectrum_col >= STREAM_BUFFER_COLS) spectrum_col = STREAM_BUFFER_COLS - 1;
            spectrum_x = STREAM_SPECTRUM_X + spectrum_col;
        }
        
        int x_diff = (spectrum_x >= 0 && axis_x >= 0) ? spectrum_x - axis_x : 999;
        
        printf("%8.0f  | %7d | %8.0f | %6d | %12d | %10d | %6d\n",
               test_freq, fft_bin, actual_bin_freq, axis_x, spectrum_col, spectrum_x, x_diff);
    }
    
    printf("\n🎯 22.5kHz Detailed Analysis:\n");
    printf("Step | Description | Value | Expected | Difference\n");
    printf("-----|-------------|-------|----------|----------\n");
    
    float freq_22_5k = 22500.0f;
    int bin_22_5k = (int)(freq_22_5k * ADC_SAMPLING_FFT_SIZE / ADC_SAMPLING_RATE + 0.5f);
    float actual_freq_from_bin = (float)bin_22_5k * ADC_SAMPLING_RATE / ADC_SAMPLING_FFT_SIZE;
    
    // Axis label position for 22.5kHz
    float normalized_axis_22_5 = (freq_22_5k - FREQUENCY_RANGE_MIN) / (FREQUENCY_RANGE_MAX - FREQUENCY_RANGE_MIN);
    int axis_x_22_5 = STREAM_SPECTRUM_X + (int)(normalized_axis_22_5 * STREAM_SPECTRUM_W);
    
    // Spectrum position for 22.5kHz signal
    float normalized_spectrum_22_5 = (actual_freq_from_bin - FREQUENCY_RANGE_MIN) / (FREQUENCY_RANGE_MAX - FREQUENCY_RANGE_MIN);
    int spectrum_col_22_5 = (int)(normalized_spectrum_22_5 * STREAM_BUFFER_COLS);
    int spectrum_x_22_5 = STREAM_SPECTRUM_X + spectrum_col_22_5;
    
    printf("  1  | Input frequency | %6.0f Hz | %6.0f Hz | %6.0f Hz\n", 
           freq_22_5k, freq_22_5k, 0.0f);
    printf("  2  | FFT bin number  | %6d    | %6d    | %6d\n", 
           bin_22_5k, bin_22_5k, 0);
    printf("  3  | Actual bin freq | %6.0f Hz | %6.0f Hz | %6.0f Hz\n", 
           actual_freq_from_bin, freq_22_5k, actual_freq_from_bin - freq_22_5k);
    printf("  4  | Axis X position | %6d px | %6d px | %6d px\n", 
           axis_x_22_5, axis_x_22_5, 0);
    printf("  5  | Spectrum column | %6d    | %6d    | %6d\n", 
           spectrum_col_22_5, spectrum_col_22_5, 0);
    printf("  6  | Spectrum X pos  | %6d px | %6d px | %6d px\n", 
           spectrum_x_22_5, axis_x_22_5, spectrum_x_22_5 - axis_x_22_5);
    
    printf("\n🔍 Display System Simulation:\n");
    printf("The 22.5kHz signal should appear at X=%d, matching axis label at X=%d\n", 
           spectrum_x_22_5, axis_x_22_5);
    printf("X position difference: %d pixels\n", spectrum_x_22_5 - axis_x_22_5);
    
    if (abs(spectrum_x_22_5 - axis_x_22_5) > 2) {
        printf("⚠️  WARNING: Spectrum and axis positions don't match!\n");
        printf("   This indicates a frequency mapping problem in the display system.\n");
    } else {
        printf("✅ OK: Spectrum and axis positions match within tolerance.\n");
        printf("   If visual display still shows wrong position, check spectrum_buffer[] values.\n");
    }
    
    printf("\n📋 Debug Note:\n");
    printf("   If 22.5kHz still appears at 25kHz position visually, the problem is likely\n");
    printf("   in the actual spectrum_buffer[].x values used by fft_streaming_display_render_buffer().\n");
    printf("   Check: spectrum_buffer[105].x should be %d but might be different.\n", spectrum_x_22_5);
    
    printf("===================================\n\n");
}

/**
 * Update display with new spectrum data
 */
void fft_realtime_unified_update_display(float* magnitude_spectrum) {
    // Debug output for frequency and amplitude mapping (only first few times)
    static int debug_count = 0;
    if (debug_count < 3) {
        // fft_realtime_unified_debug_frequency_mapping(magnitude_spectrum);
        // fft_realtime_unified_debug_amplitude_mapping(magnitude_spectrum);
        debug_count++;
    }
    
    // Convert FFT magnitude spectrum to display format
    // The existing fft_streaming_display system expects magnitude_db array and sample_rate
    
    // ⚠️ IMPORTANT: The display system expects RAW FFT magnitude array, not pre-mapped!
    // We pass the full magnitude_spectrum array directly, let display system do the mapping
    
    // Apply window correction to the entire spectrum
    static float corrected_spectrum[ADC_SAMPLING_FFT_SIZE/2];
    float window_correction = fft_realtime_unified_get_window_correction();
    
    for (int bin = 0; bin < ADC_SAMPLING_FFT_SIZE/2; bin++) {
        corrected_spectrum[bin] = magnitude_spectrum[bin] + 20.0f * log10f(window_correction);
    }
    
    // Update streaming display with RAW spectrum and correct sample rate
    fft_streaming_display_update_spectrum(corrected_spectrum, (float)ADC_SAMPLING_RATE);
}

/**
 * Print system status information
 */
void fft_realtime_unified_print_status(void) {
    printf("=== FFT Analysis Status (Frame #%lu) ===\n", frame_count);
    printf("Performance:\n");
    printf("  Actual FPS: %.1f (Target: %d)\n", actual_fps, TARGET_FPS);
    printf("  Processing Errors: %lu\n", error_count);
    
    printf("ADC Sampling:\n");
    printf("  Mode: %s\n", adc_sampling_get_mode() == ADC_MODE_DMA ? "DMA" : "Manual");
    printf("  Actual Rate: %.1f Hz (Target: %d Hz)\n", 
           adc_sampling_get_actual_rate(), SAMPLING_RATE_HZ);
    printf("  Total Samples: %lu\n", adc_sampling_get_sample_count());
    printf("  Buffer Overruns: %lu\n", adc_sampling_get_overrun_count());
    
    printf("Configuration:\n");
    printf("  Window: %s (Type=%d, Correction=%.4f)\n", 
           fft_realtime_unified_get_window_name(), FFT_WINDOW_TYPE,
           fft_realtime_unified_get_window_correction());
    printf("  Frequency Range: %d - %d Hz\n", FREQUENCY_RANGE_MIN, FREQUENCY_RANGE_MAX);
    printf("  Amplitude Range: %d to %d dBm\n", AMPLITUDE_RANGE_MIN_DB, AMPLITUDE_RANGE_MAX_DB);
    
    printf("===============================================\n");
}

/**
 * Get current window function name
 */
const char* fft_realtime_unified_get_window_name(void) {
    const char* window_names[] = {
        "Rectangle", "Hamming", "Hann", "Blackman", 
        "Blackman-Harris", "Kaiser-Bessel", "Flat-Top"
    };
    
    if (FFT_WINDOW_TYPE >= 0 && FFT_WINDOW_TYPE < 7) {
        return window_names[FFT_WINDOW_TYPE];
    }
    return "Unknown";
}

/**
 * Get current window function amplitude correction factor
 */
float fft_realtime_unified_get_window_correction(void) {
    const float window_corrections[] = {
        WINDOW_AMPLITUDE_CORRECTION_RECTANGLE,
        WINDOW_AMPLITUDE_CORRECTION_HAMMING,
        WINDOW_AMPLITUDE_CORRECTION_HANN,
        WINDOW_AMPLITUDE_CORRECTION_BLACKMAN,
        WINDOW_AMPLITUDE_CORRECTION_BLACKMANHARRIS,
        WINDOW_AMPLITUDE_CORRECTION_KAISER_BESSEL,
        WINDOW_AMPLITUDE_CORRECTION_FLATTOP
    };
    
    if (FFT_WINDOW_TYPE >= 0 && FFT_WINDOW_TYPE < 7) {
        return window_corrections[FFT_WINDOW_TYPE];
    }
    return 1.0f;  // Default correction
}

/**
 * Cleanup and shutdown unified system
 */
void fft_realtime_unified_cleanup(void) {
    printf("Shutting down unified real-time FFT analysis system...\n");
    
    // Stop ADC sampling
    adc_sampling_stop();
    
    // Print final statistics
    printf("Final Statistics:\n");
    printf("  Total Frames Processed: %lu\n", frame_count);
    printf("  Total Processing Errors: %lu\n", error_count);
    printf("  Final FPS: %.1f\n", actual_fps);
    printf("  Total Samples: %lu\n", adc_sampling_get_sample_count());
    printf("  Total Buffer Overruns: %lu\n", adc_sampling_get_overrun_count());
    
    printf("Unified system shutdown complete.\n");
}
