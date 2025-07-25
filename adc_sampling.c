/*****************************************************************************
* | File      	:   adc_sampling.c
* | Author      :   PicoFFT Project
* | Function    :   Unified ADC sampling implementation (Manual + DMA modes)
* | Info        :   
*   - Implementation of unified ADC sampling interface
*   - Support for both manual polling and DMA-based sampling
*   - Double buffering for continuous real-time processing
*   - FFT integration with configurable window functions
*----------------
******************************************************************************/

#include "adc_sampling.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

// Global unified analyzer instance
unified_fft_analyzer_t g_unified_analyzer = {0};

// ========================================
// 🔧 Core ADC Sampling API Implementation
// ========================================

/**
 * Initialize ADC sampling system
 */
bool adc_sampling_init(adc_sampling_mode_t mode) {
    printf("Initializing ADC sampling system in %s mode...\n", 
           mode == ADC_MODE_DMA ? "DMA" : "Manual");
    
    // Reset analyzer state
    memset(&g_unified_analyzer, 0, sizeof(unified_fft_analyzer_t));
    g_unified_analyzer.mode = mode;
    g_unified_analyzer.status = ADC_STATUS_IDLE;
    
    // Initialize common ADC hardware
    adc_init();
    adc_gpio_init(26);  // GP26 as ADC input
    adc_select_input(ADC_SAMPLING_CHANNEL);
    
    // Set ADC clock divider for target sampling rate
    // ADC clock = 48MHz, target = 128kHz, divider = 48MHz/128kHz = 375
    float adc_clkdiv = 48000000.0f / (float)ADC_SAMPLING_RATE;
    adc_set_clkdiv(adc_clkdiv);
    
    // Initialize buffer pointers
    g_unified_analyzer.current_buffer = g_unified_analyzer.buffer_ping;
    g_unified_analyzer.ready_buffer = NULL;
    g_unified_analyzer.buffer_selector = false;  // Start with ping buffer
    
    // Initialize kiss_fft configuration
    g_unified_analyzer.fft_cfg = kiss_fft_alloc(ADC_SAMPLING_FFT_SIZE, 0, NULL, NULL);
    if (g_unified_analyzer.fft_cfg == NULL) {
        printf("ERROR: Failed to allocate kiss_fft configuration!\n");
        return false;
    }
    
    // Initialize mode-specific components
    bool init_success = false;
    if (mode == ADC_MODE_DMA) {
        init_success = _adc_dma_init();
    } else {
        init_success = _adc_manual_init();
    }
    
    if (init_success) {
        printf("ADC sampling system initialized successfully\n");
        printf("  Mode: %s\n", mode == ADC_MODE_DMA ? "DMA" : "Manual");
        printf("  Sampling Rate: %d Hz\n", ADC_SAMPLING_RATE);
        printf("  FFT Size: %d\n", ADC_SAMPLING_FFT_SIZE);
        printf("  Buffer Size: %d samples\n", ADC_SAMPLING_FFT_SIZE);
    } else {
        printf("ERROR: Failed to initialize ADC sampling system!\n");
    }
    
    return init_success;
}

/**
 * Start ADC sampling
 */
bool adc_sampling_start(void) {
    if (g_unified_analyzer.status == ADC_STATUS_SAMPLING) {
        printf("Warning: ADC sampling already active\n");
        return true;
    }
    
    printf("Starting ADC sampling...\n");
    
    // Reset performance counters
    adc_sampling_reset_counters();
    g_unified_analyzer.sampling_start_time = get_absolute_time();
    
    // Start mode-specific sampling
    if (g_unified_analyzer.mode == ADC_MODE_DMA) {
        _adc_dma_start();
    }
    // Manual mode starts sampling on first call to adc_sampling_is_ready()
    
    g_unified_analyzer.status = ADC_STATUS_SAMPLING;
    g_unified_analyzer.sampling_active = true;
    
    printf("ADC sampling started in %s mode\n", 
           g_unified_analyzer.mode == ADC_MODE_DMA ? "DMA" : "Manual");
    return true;
}

/**
 * Stop ADC sampling
 */
bool adc_sampling_stop(void) {
    if (g_unified_analyzer.status == ADC_STATUS_IDLE) {
        printf("Warning: ADC sampling already stopped\n");
        return true;
    }
    
    printf("Stopping ADC sampling...\n");
    
    // Stop mode-specific sampling
    if (g_unified_analyzer.mode == ADC_MODE_DMA) {
        _adc_dma_stop();
    }
    
    g_unified_analyzer.status = ADC_STATUS_IDLE;
    g_unified_analyzer.sampling_active = false;
    g_unified_analyzer.data_ready = false;
    
    // Calculate final sampling rate
    absolute_time_t total_time = absolute_time_diff_us(
        g_unified_analyzer.sampling_start_time, get_absolute_time());
    if (total_time > 0) {
        g_unified_analyzer.actual_sample_rate = 
            (float)g_unified_analyzer.sample_count * 1000000.0f / (float)total_time;
    }
    
    printf("ADC sampling stopped\n");
    printf("  Total samples: %lu\n", g_unified_analyzer.sample_count);
    printf("  Actual rate: %.1f Hz\n", g_unified_analyzer.actual_sample_rate);
    printf("  Buffer overruns: %lu\n", g_unified_analyzer.buffer_overruns);
    
    return true;
}

/**
 * Check if new ADC data is ready for processing
 */
bool adc_sampling_is_ready(void) {
    // For manual mode, perform sampling here
    if (g_unified_analyzer.mode == ADC_MODE_MANUAL && 
        g_unified_analyzer.sampling_active && 
        !g_unified_analyzer.data_ready) {
        _adc_manual_sample_buffer();
    }
    
    return g_unified_analyzer.data_ready;
}

/**
 * Get pointer to buffer with ready ADC data
 */
uint16_t* adc_sampling_get_buffer(void) {
    if (!g_unified_analyzer.data_ready) {
        return NULL;
    }
    
    return g_unified_analyzer.ready_buffer;
}

/**
 * Signal that processing of current buffer is complete
 */
void adc_sampling_complete_processing(void) {
    if (g_unified_analyzer.data_ready) {
        g_unified_analyzer.data_ready = false;
        g_unified_analyzer.fft_ready = false;
        g_unified_analyzer.ready_buffer = NULL;
        
        // For manual mode, we can immediately start next buffer
        // For DMA mode, next buffer is already being filled automatically
    }
}

/**
 * Get current sampling status
 */
adc_sampling_status_t adc_sampling_get_status(void) {
    return g_unified_analyzer.status;
}

/**
 * Get current sampling mode
 */
adc_sampling_mode_t adc_sampling_get_mode(void) {
    return g_unified_analyzer.mode;
}

// ========================================
// 🔧 Performance Monitoring API Implementation
// ========================================

/**
 * Get actual measured sampling rate
 */
float adc_sampling_get_actual_rate(void) {
    return g_unified_analyzer.actual_sample_rate;
}

/**
 * Get buffer overrun count
 */
uint32_t adc_sampling_get_overrun_count(void) {
    return g_unified_analyzer.buffer_overruns;
}

/**
 * Get total sample count
 */
uint32_t adc_sampling_get_sample_count(void) {
    return g_unified_analyzer.sample_count;
}

/**
 * Reset performance counters
 */
void adc_sampling_reset_counters(void) {
    g_unified_analyzer.sample_count = 0;
    g_unified_analyzer.buffer_overruns = 0;
    g_unified_analyzer.actual_sample_rate = 0.0f;
}

// ========================================
// 🔧 FFT Integration API Implementation
// ========================================

/**
 * Process current ADC buffer through FFT with windowing
 */
bool adc_sampling_process_fft(void) {
    uint16_t* buffer = adc_sampling_get_buffer();
    if (buffer == NULL) {
        return false;
    }
    
    // Apply window function and convert to complex
    _adc_apply_window_function(buffer, g_unified_analyzer.fft_input);
    
    // Perform FFT
    kiss_fft(g_unified_analyzer.fft_cfg, 
             g_unified_analyzer.fft_input, 
             g_unified_analyzer.fft_output);
    
    // Calculate magnitude spectrum
    for (int i = 0; i < ADC_SAMPLING_FFT_SIZE/2; i++) {
        float real = g_unified_analyzer.fft_output[i].r;
        float imag = g_unified_analyzer.fft_output[i].i;
        
        // Calculate magnitude
        float magnitude = sqrtf(real * real + imag * imag);
        
        // Normalize by FFT size
        magnitude /= ADC_SAMPLING_FFT_SIZE;
        
        // Convert ADC digital magnitude to voltage
        // ADC reading (0-4095) -> voltage (0-3.3V)
        float voltage_magnitude = magnitude * ADC_VOLTAGE_PER_BIT;
        
        // Convert to dBm scale relative to 0.274V @ 0dBm for 75Ω system
        // dBm = 20 * log10(Vmeasured / V0dBm)
        float db_magnitude;
        if (voltage_magnitude > 1e-10f) {
            db_magnitude = 20.0f * log10f(voltage_magnitude / DB_REFERENCE_VOLTAGE_0DBM);
        } else {
            db_magnitude = -200.0f;  // Very low value
        }
        
        g_unified_analyzer.magnitude[i] = db_magnitude;
    }
    
    g_unified_analyzer.fft_ready = true;
    return true;
}

/**
 * Check if FFT results are ready
 */
bool adc_sampling_is_fft_ready(void) {
    return g_unified_analyzer.fft_ready;
}

/**
 * Get FFT magnitude spectrum
 */
float* adc_sampling_get_magnitude_spectrum(void) {
    if (!g_unified_analyzer.fft_ready) {
        return NULL;
    }
    return g_unified_analyzer.magnitude;
}

/**
 * Convert FFT bin to frequency in Hz
 */
float adc_sampling_bin_to_frequency(int bin) {
    return (float)bin * ADC_SAMPLING_RATE / (float)ADC_SAMPLING_FFT_SIZE;
}

// ========================================
// 🔧 DMA Mode Implementation
// ========================================

/**
 * Initialize DMA mode
 */
bool _adc_dma_init(void) {
    #if ADC_DMA_ENABLED
    printf("Initializing DMA mode...\n");
    
    // Configure ADC for DMA mode
    adc_set_round_robin(0);  // Single channel
    adc_fifo_setup(true, true, 1, false, false);  // Enable FIFO, enable DMA requests
    
    // Claim DMA channel
    if (ADC_DMA_CHANNEL_AUTO == -1) {
        g_unified_analyzer.dma_channel = dma_claim_unused_channel(true);
    } else {
        g_unified_analyzer.dma_channel = ADC_DMA_CHANNEL_AUTO;
        dma_channel_claim(g_unified_analyzer.dma_channel);
    }
    
    if (g_unified_analyzer.dma_channel < 0) {
        printf("ERROR: Failed to claim DMA channel\n");
        return false;
    }
    
    // Configure DMA channel
    g_unified_analyzer.dma_config = dma_channel_get_default_config(g_unified_analyzer.dma_channel);
    channel_config_set_transfer_data_size(&g_unified_analyzer.dma_config, ADC_DMA_TRANSFER_SIZE);
    channel_config_set_read_increment(&g_unified_analyzer.dma_config, false);   // Read from ADC FIFO
    channel_config_set_write_increment(&g_unified_analyzer.dma_config, true);   // Write to buffer
    channel_config_set_dreq(&g_unified_analyzer.dma_config, DREQ_ADC);         // ADC triggers DMA
    
    // Set up DMA interrupt
    dma_channel_set_irq0_enabled(g_unified_analyzer.dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, _adc_dma_interrupt_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    irq_set_priority(DMA_IRQ_0, ADC_DMA_PRIORITY);
    
    printf("DMA mode initialized - Channel: %d\n", g_unified_analyzer.dma_channel);
    return true;
    #else
    printf("ERROR: DMA mode not enabled in config_settings.h\n");
    return false;
    #endif
}

/**
 * Start DMA sampling
 */
void _adc_dma_start(void) {
    #if ADC_DMA_ENABLED
    // Configure first DMA transfer
    dma_channel_configure(
        g_unified_analyzer.dma_channel,
        &g_unified_analyzer.dma_config,
        g_unified_analyzer.current_buffer,    // Destination
        &adc_hw->fifo,                        // Source (ADC FIFO)
        ADC_SAMPLING_FFT_SIZE,                // Transfer count
        false                                 // Don't start yet
    );
    
    // Start DMA and ADC
    dma_channel_start(g_unified_analyzer.dma_channel);
    adc_run(true);
    
    printf("DMA sampling started\n");
    #endif
}

/**
 * Stop DMA sampling
 */
void _adc_dma_stop(void) {
    #if ADC_DMA_ENABLED
    adc_run(false);
    dma_channel_abort(g_unified_analyzer.dma_channel);
    printf("DMA sampling stopped\n");
    #endif
}

/**
 * DMA interrupt handler
 */
void _adc_dma_interrupt_handler(void) {
    #if ADC_DMA_ENABLED
    // Clear interrupt flag
    dma_hw->ints0 = 1u << g_unified_analyzer.dma_channel;
    
    // Update sample count
    g_unified_analyzer.sample_count += ADC_SAMPLING_FFT_SIZE;
    
    // Check for buffer overrun
    if (g_unified_analyzer.data_ready) {
        g_unified_analyzer.buffer_overruns++;
        if (ADC_DMA_OVERRUN_DETECTION) {
            printf("Warning: Buffer overrun detected!\n");
        }
    }
    
    // Swap buffers
    _adc_swap_buffers();
    
    // Set up next DMA transfer to new current buffer
    dma_channel_configure(
        g_unified_analyzer.dma_channel,
        &g_unified_analyzer.dma_config,
        g_unified_analyzer.current_buffer,    // New destination buffer
        &adc_hw->fifo,                        // Source (ADC FIFO)
        ADC_SAMPLING_FFT_SIZE,                // Transfer count
        true                                  // Start immediately
    );
    
    // Mark data as ready
    g_unified_analyzer.data_ready = true;
    g_unified_analyzer.last_buffer_completion = get_absolute_time();
    #endif
}

// ========================================
// 🔧 Manual Mode Implementation
// ========================================

/**
 * Initialize manual mode
 */
bool _adc_manual_init(void) {
    printf("Initializing manual mode...\n");
    
    // No special initialization needed for manual mode
    g_unified_analyzer.manual_sample_index = 0;
    
    printf("Manual mode initialized\n");
    return true;
}

/**
 * Sample complete buffer manually
 */
void _adc_manual_sample_buffer(void) {
    // Record sampling start time
    absolute_time_t sample_start = get_absolute_time();
    
    // Sample ADC data with precise timing
    for (int i = 0; i < ADC_SAMPLING_FFT_SIZE; i++) {
        g_unified_analyzer.current_buffer[i] = adc_read();
        
        // Precise timing for target sampling rate
        sleep_us(SAMPLING_INTERVAL_US);
    }
    
    // Update sample count
    g_unified_analyzer.sample_count += ADC_SAMPLING_FFT_SIZE;
    
    // Calculate actual sampling rate
    absolute_time_t sample_end = get_absolute_time();
    int64_t sample_time_us = absolute_time_diff_us(sample_start, sample_end);
    if (sample_time_us > 0) {
        float current_rate = (float)ADC_SAMPLING_FFT_SIZE * 1000000.0f / (float)sample_time_us;
        
        // Exponential moving average for stable rate measurement
        if (g_unified_analyzer.actual_sample_rate == 0.0f) {
            g_unified_analyzer.actual_sample_rate = current_rate;
        } else {
            g_unified_analyzer.actual_sample_rate = 
                0.9f * g_unified_analyzer.actual_sample_rate + 0.1f * current_rate;
        }
    }
    
    // Swap buffers and mark data ready
    _adc_swap_buffers();
    g_unified_analyzer.data_ready = true;
    g_unified_analyzer.last_buffer_completion = get_absolute_time();
}

// ========================================
// 🔧 Common Internal Functions
// ========================================

/**
 * Swap ping-pong buffers
 */
void _adc_swap_buffers(void) {
    // Current buffer becomes ready buffer
    g_unified_analyzer.ready_buffer = g_unified_analyzer.current_buffer;
    
    // Switch to other buffer for next sampling
    g_unified_analyzer.buffer_selector = !g_unified_analyzer.buffer_selector;
    g_unified_analyzer.current_buffer = g_unified_analyzer.buffer_selector ? 
        g_unified_analyzer.buffer_pong : g_unified_analyzer.buffer_ping;
}

/**
 * Apply window function and convert ADC data to complex FFT input
 */
void _adc_apply_window_function(uint16_t* adc_buffer, kiss_fft_cpx* fft_input) {
    // Calculate DC offset for removal
    float dc_offset = 0.0f;
    for (int i = 0; i < ADC_SAMPLING_FFT_SIZE; i++) {
        dc_offset += (float)adc_buffer[i];
    }
    dc_offset /= ADC_SAMPLING_FFT_SIZE;
    
    // Apply window function based on config_settings.h configuration
    for (int i = 0; i < ADC_SAMPLING_FFT_SIZE; i++) {
        // Remove DC offset and convert to float
        float sample = (float)adc_buffer[i] - dc_offset;
        
        // Apply selected window function
        float window = 1.0f;  // Default: Rectangle window
        
        switch (FFT_WINDOW_TYPE) {
            case 0:  // Rectangle
                window = 1.0f;
                break;
            case 1:  // Hamming
                window = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (ADC_SAMPLING_FFT_SIZE - 1));
                break;
            case 2:  // Hann
                window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (ADC_SAMPLING_FFT_SIZE - 1)));
                break;
            case 3:  // Blackman
                window = 0.42f - 0.5f * cosf(2.0f * M_PI * i / (ADC_SAMPLING_FFT_SIZE - 1)) + 
                         0.08f * cosf(4.0f * M_PI * i / (ADC_SAMPLING_FFT_SIZE - 1));
                break;
            case 4:  // Blackman-Harris
                window = 0.35875f - 0.48829f * cosf(2.0f * M_PI * i / (ADC_SAMPLING_FFT_SIZE - 1)) + 
                         0.14128f * cosf(4.0f * M_PI * i / (ADC_SAMPLING_FFT_SIZE - 1)) - 
                         0.01168f * cosf(6.0f * M_PI * i / (ADC_SAMPLING_FFT_SIZE - 1));
                break;
            case 5:  // Kaiser-Bessel (beta = 8.5)
                {
                    float alpha = (ADC_SAMPLING_FFT_SIZE - 1) / 2.0f;
                    float x = (i - alpha) / alpha;
                    float arg = KAISER_BESSEL_BETA * sqrtf(1.0f - x * x);
                    // Simplified Kaiser window approximation
                    window = (arg < 50.0f) ? expf(arg - KAISER_BESSEL_BETA) : 0.0f;
                }
                break;
            case 6:  // Flat-Top
                window = 1.0f - 1.93f * cosf(2.0f * M_PI * i / (ADC_SAMPLING_FFT_SIZE - 1)) + 
                         1.29f * cosf(4.0f * M_PI * i / (ADC_SAMPLING_FFT_SIZE - 1)) - 
                         0.388f * cosf(6.0f * M_PI * i / (ADC_SAMPLING_FFT_SIZE - 1)) + 
                         0.032f * cosf(8.0f * M_PI * i / (ADC_SAMPLING_FFT_SIZE - 1));
                break;
            default:
                window = 1.0f;  // Default to rectangle
                break;
        }
        
        // Apply window and store in FFT input buffer
        sample *= window;
        fft_input[i].r = sample;
        fft_input[i].i = 0.0f;
    }
}
