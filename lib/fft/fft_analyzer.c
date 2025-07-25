/*****************************************************************************
* | File      	:   fft_analyzer.c
* | Author      :   
* | Function    :   FFT spectrum analyzer implementation
* | Info        :   
*   - Cooley-Tukey FFT algorithm
*   - ADC DMA sampling
*   - Spectrum display
*----------------
******************************************************************************/

#include "fft_analyzer.h"
#include "config_settings.h"  // 窓関数設定のため

// Global FFT analyzer instance
fft_analyzer_t g_fft_analyzer = {0};

// DMA variables
static int dma_channel;
static dma_channel_config dma_config;

// Timer for sampling rate control
static repeating_timer_t sampling_timer;

// Forward declarations
static void dma_handler(void);
static bool sampling_timer_callback(repeating_timer_t *rt);

/**
 * Initialize FFT analyzer
 */
void fft_analyzer_init(void) {
    // Initialize ADC
    adc_init();
    adc_gpio_init(26); // GP26 as ADC input
    adc_select_input(ADC_CHANNEL);
    
    // Set ADC to 12-bit resolution and configure for free-running mode
    adc_set_round_robin(0);
    adc_fifo_setup(true, true, 1, false, false); // Enable FIFO, enable DMA requests
    
    // Set ADC clock divider for 128kHz sampling
    // ADC runs at 48MHz by default, so we need divider of 375 for 128kHz
    adc_set_clkdiv(375.0f);
    
    // Initialize DMA
    dma_channel = dma_claim_unused_channel(true);
    dma_config = dma_channel_get_default_config(dma_channel);
    
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_16);
    channel_config_set_read_increment(&dma_config, false);
    channel_config_set_write_increment(&dma_config, true);
    channel_config_set_dreq(&dma_config, DREQ_ADC);
    
    // Set up DMA interrupt
    dma_channel_set_irq0_enabled(dma_channel, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    
    // Initialize frequency bins
    for (int i = 0; i < FFT_SIZE/2; i++) {
        g_fft_analyzer.freq_bins[i] = fft_bin_to_frequency(i);
    }
    
    // Initialize data arrays
    g_fft_analyzer.data_ready = false;
    
    printf("FFT Analyzer initialized - Sampling at %d Hz\n", SAMPLE_RATE);
}

/**
 * Start ADC sampling with DMA
 */
void fft_analyzer_start_sampling(void) {
    g_fft_analyzer.data_ready = false;
    
    // Configure DMA for circular buffer operation
    dma_channel_configure(
        dma_channel,
        &dma_config,
        g_fft_analyzer.adc_buffer,    // Destination
        &adc_hw->fifo,                // Source
        FFT_SIZE,                     // Transfer count
        false                         // Don't start yet
    );
    
    // Start DMA transfer
    dma_channel_start(dma_channel);
    
    // Start ADC in free-running mode
    adc_run(true);
    
    printf("FFT sampling started - DMA active\n");
}

/**
 * Stop ADC sampling
 */
void fft_analyzer_stop_sampling(void) {
    adc_run(false);
    dma_channel_abort(dma_channel);
    printf("FFT sampling stopped\n");
}

/**
 * DMA interrupt handler - called when ADC buffer is full
 */
static void dma_handler(void) {
    // Clear the interrupt flag
    dma_hw->ints0 = 1u << dma_channel;
    
    // Signal that new data is ready for processing
    g_fft_analyzer.data_ready = true;
    
    // Restart DMA for continuous sampling (circular buffer)
    dma_channel_configure(
        dma_channel,
        &dma_config,
        g_fft_analyzer.adc_buffer,
        &adc_hw->fifo,
        FFT_SIZE,
        true  // Start immediately
    );
    
    // Optional: Toggle LED or GPIO to indicate data capture
    // gpio_put(PICO_DEFAULT_LED_PIN, !gpio_get(PICO_DEFAULT_LED_PIN));
}

/**
 * Cooley-Tukey FFT algorithm (radix-2)
 */
void fft_compute(complex_t* data, int n) {
    // Bit-reversal permutation
    int j = 0;
    for (int i = 1; i < n; i++) {
        int bit = n >> 1;
        while (j & bit) {
            j ^= bit;
            bit >>= 1;
        }
        j ^= bit;
        
        if (i < j) {
            complex_t temp = data[i];
            data[i] = data[j];
            data[j] = temp;
        }
    }
    
    // FFT computation
    for (int len = 2; len <= n; len <<= 1) {
        float angle = -2.0 * M_PI / len;
        complex_t wlen = {cos(angle), sin(angle)};
        
        for (int i = 0; i < n; i += len) {
            complex_t w = {1.0, 0.0};
            
            for (int j = 0; j < len / 2; j++) {
                complex_t u = data[i + j];
                complex_t v = {
                    data[i + j + len/2].real * w.real - data[i + j + len/2].imag * w.imag,
                    data[i + j + len/2].real * w.imag + data[i + j + len/2].imag * w.real
                };
                
                data[i + j] = (complex_t){u.real + v.real, u.imag + v.imag};
                data[i + j + len/2] = (complex_t){u.real - v.real, u.imag - v.imag};
                
                // Update w
                float w_temp = w.real * wlen.real - w.imag * wlen.imag;
                w.imag = w.real * wlen.imag + w.imag * wlen.real;
                w.real = w_temp;
            }
        }
    }
}

/**
 * Process ADC data and compute FFT
 */
void fft_process_data(void) {
    if (!g_fft_analyzer.data_ready) {
        return;
    }
    
    // Convert ADC data to complex numbers and apply selected window function
    for (int i = 0; i < FFT_SIZE; i++) {
        // Calculate window function based on config_settings.h configuration
        float window = 1.0f;  // Default: Rectangle window
        
        switch (FFT_WINDOW_TYPE) {
            case 0: // Rectangle window
                window = 1.0f;
                break;
            case 1: // Hamming window
                window = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1));
                break;
            case 2: // Hann window (Hanning)
                window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
                break;
            case 3: // Blackman window
                window = 0.42f - 0.5f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1)) + 0.08f * cosf(4.0f * M_PI * i / (FFT_SIZE - 1));
                break;
            case 4: // Blackman-Harris window
                window = 0.35875f - 0.48829f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1)) + 0.14128f * cosf(4.0f * M_PI * i / (FFT_SIZE - 1)) - 0.01168f * cosf(6.0f * M_PI * i / (FFT_SIZE - 1));
                break;
            case 5: // Kaiser-Bessel window (β=8.5)
                // Simplified Kaiser-Bessel implementation
                {
                    float n = i - (FFT_SIZE - 1) / 2.0f;
                    float alpha = (FFT_SIZE - 1) / 2.0f;
                    float arg = KAISER_BESSEL_BETA * sqrtf(1.0f - (n / alpha) * (n / alpha));
                    window = 0.4f * (1.0f + 0.6f * arg / KAISER_BESSEL_BETA);  // Simplified approximation
                }
                break;
            case 6: // Flat-Top window
                window = 0.21557895f - 0.41663158f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1)) + 0.277263158f * cosf(4.0f * M_PI * i / (FFT_SIZE - 1)) - 0.083578947f * cosf(6.0f * M_PI * i / (FFT_SIZE - 1)) + 0.006947368f * cosf(8.0f * M_PI * i / (FFT_SIZE - 1));
                break;
            default:
                window = 1.0f; // Fall back to Rectangle window
                break;
        }
        
        // Convert ADC value (0-4095) to normalized range (-1 to +1)
        float sample = (g_fft_analyzer.adc_buffer[i] - 2048.0f) / 2048.0f;
        
        // Apply window and store in FFT buffer
        g_fft_analyzer.fft_buffer[i].real = sample * window;
        g_fft_analyzer.fft_buffer[i].imag = 0.0f;
    }
    
    // Compute FFT
    fft_compute(g_fft_analyzer.fft_buffer, FFT_SIZE);
    
    // Calculate magnitude spectrum with logarithmic scale
    for (int i = 0; i < FFT_SIZE/2; i++) {
        float real = g_fft_analyzer.fft_buffer[i].real;
        float imag = g_fft_analyzer.fft_buffer[i].imag;
        float magnitude = sqrtf(real*real + imag*imag);
        
        // Convert to dB scale with noise floor
        if (magnitude > 1e-10f) {
            g_fft_analyzer.magnitude[i] = 20.0f * log10f(magnitude + 1e-10f);
        } else {
            g_fft_analyzer.magnitude[i] = -200.0f; // Very low noise floor
        }
        
        // Clamp to reasonable range
        if (g_fft_analyzer.magnitude[i] < -100.0f) {
            g_fft_analyzer.magnitude[i] = -100.0f;
        }
        if (g_fft_analyzer.magnitude[i] > 0.0f) {
            g_fft_analyzer.magnitude[i] = 0.0f;
        }
    }
    
    // Prepare spectrum data for display (filter frequency range 100Hz - 50kHz)
    int display_index = 0;
    for (int i = 0; i < FFT_SIZE/2 && display_index < FREQ_BINS; i++) {
        float freq = g_fft_analyzer.freq_bins[i];
        if (freq >= MIN_FREQ && freq <= MAX_FREQ) {
            // Normalize magnitude to display range (0-255)
            // Map -100dB to 0dB range to 0-255
            float normalized = (g_fft_analyzer.magnitude[i] + 100.0f) / 100.0f;
            if (normalized < 0.0f) normalized = 0.0f;
            if (normalized > 1.0f) normalized = 1.0f;
            
            g_fft_analyzer.spectrum_data[display_index] = (uint16_t)(normalized * 255);
            display_index++;
            
            // Stop when we have enough bins for display
            if (display_index >= FREQ_BINS) break;
        }
    }
    
    // Fill remaining bins if needed
    while (display_index < FREQ_BINS) {
        g_fft_analyzer.spectrum_data[display_index] = 0;
        display_index++;
    }
    
    g_fft_analyzer.data_ready = false;
}

/**
 * Convert FFT bin to frequency
 */
float fft_bin_to_frequency(int bin) {
    return (float)bin * SAMPLE_RATE / FFT_SIZE;
}

/**
 * Convert frequency to display X coordinate
 */
int fft_frequency_to_display_x(float freq) {
    if (freq < MIN_FREQ) freq = MIN_FREQ;
    if (freq > MAX_FREQ) freq = MAX_FREQ;
    
    // Logarithmic scale
    float log_min = log10(MIN_FREQ);
    float log_max = log10(MAX_FREQ);
    float log_freq = log10(freq);
    
    float normalized = (log_freq - log_min) / (log_max - log_min);
    return SPECTRUM_X_OFFSET + (int)(normalized * SPECTRUM_WIDTH);
}

/**
 * Convert magnitude to display Y coordinate
 */
int fft_magnitude_to_display_y(float magnitude) {
    // Magnitude is already normalized (0-255)
    float normalized = magnitude / 255.0;
    return SPECTRUM_Y_OFFSET + SPECTRUM_HEIGHT - (int)(normalized * SPECTRUM_HEIGHT);
}
