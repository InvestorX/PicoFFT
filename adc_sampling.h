/*****************************************************************************
* | File      	:   adc_sampling.h
* | Author      :   PicoFFT Project
* | Function    :   Unified ADC sampling interface (Manual + DMA modes)
* | Info        :   
*   - Abstraction layer for ADC sampling methods
*   - Support for both manual polling and DMA-based sampling
*   - Double buffering for continuous real-time processing
*   - Configurable via config_settings.h
*----------------
******************************************************************************/

#ifndef __ADC_SAMPLING_H
#define __ADC_SAMPLING_H

#include <stdint.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "kiss_fft.h"
#include "config_settings.h"
#include "lib/fft/fft_analyzer.h"

// ADC sampling configuration from config_settings.h
#define ADC_SAMPLING_FFT_SIZE 1024          // Must match FFT processing size
#define ADC_SAMPLING_RATE SAMPLING_RATE_HZ  // 128kHz from config_settings.h
#define ADC_SAMPLING_CHANNEL 0              // GP26 = ADC0

// ADC sampling modes
typedef enum {
    ADC_MODE_MANUAL = 0,    // Manual polling with sleep_us() timing
    ADC_MODE_DMA = 1        // DMA-based automatic sampling
} adc_sampling_mode_t;

// ADC sampling status
typedef enum {
    ADC_STATUS_IDLE = 0,        // Not sampling
    ADC_STATUS_SAMPLING = 1,    // Currently sampling
    ADC_STATUS_DATA_READY = 2,  // Data ready for processing
    ADC_STATUS_ERROR = 3        // Error state
} adc_sampling_status_t;

// Unified ADC sampling data structure
typedef struct {
    // Current configuration
    adc_sampling_mode_t mode;
    adc_sampling_status_t status;
    
    // Buffer management (double buffering)
    uint16_t buffer_ping[ADC_SAMPLING_FFT_SIZE];  // Buffer A
    uint16_t buffer_pong[ADC_SAMPLING_FFT_SIZE];  // Buffer B
    uint16_t* current_buffer;                     // Currently filling buffer
    uint16_t* ready_buffer;                       // Buffer ready for processing
    volatile bool buffer_selector;                // 0=ping active, 1=pong active
    
    // Sampling control
    volatile bool data_ready;                     // New data available flag
    volatile bool sampling_active;                // Sampling in progress flag
    volatile uint32_t sample_count;               // Total samples collected
    volatile uint32_t buffer_overruns;            // Buffer overrun counter
    
    // DMA specific (only used in DMA mode)
    int dma_channel;                              // Claimed DMA channel
    dma_channel_config dma_config;                // DMA configuration
    volatile bool dma_error;                      // DMA error flag
    
    // Manual sampling specific (only used in manual mode)
    absolute_time_t last_sample_time;             // Last sample timestamp
    uint32_t manual_sample_index;                 // Current sample index
    
    // FFT integration
    kiss_fft_cpx fft_input[ADC_SAMPLING_FFT_SIZE];   // FFT input buffer
    kiss_fft_cpx fft_output[ADC_SAMPLING_FFT_SIZE];  // FFT output buffer
    kiss_fft_cfg fft_cfg;                            // kiss_fft configuration
    float magnitude[ADC_SAMPLING_FFT_SIZE/2];        // Magnitude spectrum
    bool fft_ready;                                  // FFT results available
    
    // Performance monitoring
    absolute_time_t sampling_start_time;          // Sampling start timestamp
    absolute_time_t last_buffer_completion;       // Last buffer completion time
    float actual_sample_rate;                     // Measured sampling rate
    
} unified_fft_analyzer_t;

// Global instance
extern unified_fft_analyzer_t g_unified_analyzer;

// ========================================
// 🔧 Core ADC Sampling API
// ========================================

/**
 * Initialize ADC sampling system
 * @param mode ADC_MODE_MANUAL or ADC_MODE_DMA
 * @return true if successful, false on error
 */
bool adc_sampling_init(adc_sampling_mode_t mode);

/**
 * Start ADC sampling
 * @return true if successful, false on error
 */
bool adc_sampling_start(void);

/**
 * Stop ADC sampling
 * @return true if successful, false on error
 */
bool adc_sampling_stop(void);

/**
 * Check if new ADC data is ready for processing
 * @return true if data ready, false otherwise
 */
bool adc_sampling_is_ready(void);

/**
 * Get pointer to buffer with ready ADC data
 * @return Pointer to ready buffer, NULL if no data ready
 */
uint16_t* adc_sampling_get_buffer(void);

/**
 * Signal that processing of current buffer is complete
 * This allows the system to prepare the next buffer
 */
void adc_sampling_complete_processing(void);

/**
 * Get current sampling status
 * @return Current ADC sampling status
 */
adc_sampling_status_t adc_sampling_get_status(void);

/**
 * Get current sampling mode
 * @return Current ADC sampling mode
 */
adc_sampling_mode_t adc_sampling_get_mode(void);

// ========================================
// 🔧 Performance Monitoring API
// ========================================

/**
 * Get actual measured sampling rate
 * @return Sampling rate in Hz
 */
float adc_sampling_get_actual_rate(void);

/**
 * Get buffer overrun count
 * @return Number of buffer overruns since start
 */
uint32_t adc_sampling_get_overrun_count(void);

/**
 * Get total sample count
 * @return Total samples collected since start
 */
uint32_t adc_sampling_get_sample_count(void);

/**
 * Reset performance counters
 */
void adc_sampling_reset_counters(void);

// ========================================
// 🔧 FFT Integration API
// ========================================

/**
 * Process current ADC buffer through FFT with windowing
 * @return true if FFT completed successfully, false on error
 */
bool adc_sampling_process_fft(void);

/**
 * Check if FFT results are ready
 * @return true if FFT results available, false otherwise
 */
bool adc_sampling_is_fft_ready(void);

/**
 * Get FFT magnitude spectrum
 * @return Pointer to magnitude array (FFT_SIZE/2 elements)
 */
float* adc_sampling_get_magnitude_spectrum(void);

/**
 * Convert FFT bin to frequency in Hz
 * @param bin FFT bin index (0 to FFT_SIZE/2-1)
 * @return Frequency in Hz
 */
float adc_sampling_bin_to_frequency(int bin);

// ========================================
// 🔧 Internal Functions (Implementation Use Only)
// ========================================

// DMA mode internal functions
bool _adc_dma_init(void);
void _adc_dma_start(void);
void _adc_dma_stop(void);
void _adc_dma_interrupt_handler(void);

// Manual mode internal functions
bool _adc_manual_init(void);
void _adc_manual_sample_buffer(void);

// Common internal functions
void _adc_swap_buffers(void);
void _adc_apply_window_function(uint16_t* adc_buffer, kiss_fft_cpx* fft_input);

#endif // __ADC_SAMPLING_H
