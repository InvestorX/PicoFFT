/*****************************************************************************
* | File      	:   fft_analyzer_kiss.c
* | Author      :   
* | Function    :   FFT spectrum analyzer implementation using kiss_fft
* | Info        :   
*   - Real-time FFT analysis using kiss_fft library
*   - ADC sampling and FFT processing
*   - Magnitude spectrum calculation in dB
*----------------
******************************************************************************/

#include "fft_analyzer.h"
#include "config_settings.h"

// Global FFT analyzer instance
fft_analyzer_t g_fft_analyzer = {0};

/**
 * Initialize FFT analyzer system with kiss_fft
 */
void fft_analyzer_init(void) {
    printf("Initializing FFT analyzer with kiss_fft...\n");
    
    // Initialize ADC
    adc_init();
    adc_gpio_init(26);  // GP26 for ADC0
    adc_select_input(ADC_CHANNEL);
    
    // Set ADC clock divider for 128kHz sampling
    // ADC clock = 48MHz, divider = 48MHz / 128kHz = 375
    adc_set_clkdiv(390.0f);  // Slight adjustment for more accurate timing
    
    // Configure ADC for continuous sampling
    adc_set_round_robin(0);  // Single channel only
    adc_fifo_setup(
        true,    // Enable FIFO
        false,   // Disable DMA requests (manual sampling)
        1,       // FIFO threshold
        false,   // No error on FIFO overflow
        false    // 12-bit samples
    );
    
    // Initialize kiss_fft configuration
    g_fft_analyzer.fft_cfg = kiss_fft_alloc(FFT_SIZE, 0, NULL, NULL);
    if (g_fft_analyzer.fft_cfg == NULL) {
        printf("ERROR: Failed to allocate kiss_fft configuration!\n");
        return;
    }
    
    // Initialize frequency bins
    for (int i = 0; i < FFT_SIZE/2; i++) {
        g_fft_analyzer.freq_bins[i] = fft_bin_to_frequency(i);
    }
    
    // Initialize data arrays
    g_fft_analyzer.data_ready = false;
    
    printf("kiss_fft FFT Analyzer initialized - Sampling at %d Hz\n", SAMPLE_RATE);
}

/**
 * Start continuous ADC sampling
 */
void fft_analyzer_start_sampling(void) {
    printf("Starting FFT sampling...\n");
    adc_run(true);
}

/**
 * Stop ADC sampling
 */
void fft_analyzer_stop_sampling(void) {
    printf("Stopping FFT sampling...\n");
    adc_run(false);
}

/**
 * Process ADC data through FFT using kiss_fft
 */
void fft_process_data(void) {
    // Sample ADC data
    for (int i = 0; i < FFT_SIZE; i++) {
        // Wait for ADC conversion
        while (adc_fifo_is_empty()) {
            tight_loop_contents();
        }
        g_fft_analyzer.adc_buffer[i] = adc_fifo_get();
    }
    
    // Calculate DC offset for removal
    float dc_offset = 0;
    for (int i = 0; i < FFT_SIZE; i++) {
        dc_offset += g_fft_analyzer.adc_buffer[i];
    }
    dc_offset /= FFT_SIZE;
    
    // Prepare FFT input with windowing and DC removal
    for (int i = 0; i < FFT_SIZE; i++) {
        // Remove DC offset and convert to float
        float sample = (float)(g_fft_analyzer.adc_buffer[i]) - dc_offset;
        
        // Apply selected window function
        float window;
        switch(FFT_WINDOW_TYPE) {
            case 0:  // Rectangle
                window = 1.0f;
                break;
            case 1:  // Hamming
                window = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1));
                break;
            case 2:  // Hann
                window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
                break;
            case 3:  // Blackman
                window = 0.42f - 0.5f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1)) + 0.08f * cosf(4.0f * M_PI * i / (FFT_SIZE - 1));
                break;
            case 4:  // Blackman-Harris
                window = 0.35875f - 0.48829f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1)) + 0.14128f * cosf(4.0f * M_PI * i / (FFT_SIZE - 1)) - 0.01168f * cosf(6.0f * M_PI * i / (FFT_SIZE - 1));
                break;
            case 5:  // Kaiser-Bessel (beta = 8.6)
                {
                    float alpha = (FFT_SIZE - 1) / 2.0f;
                    float x = (i - alpha) / alpha;
                    float arg = 8.6f * sqrtf(1.0f - x * x);
                    // Simplified Kaiser window approximation
                    window = (arg < 50.0f) ? expf(arg - 8.6f) : 0.0f;
                }
                break;
            case 6:  // Flat-Top
                window = 1.0f - 1.93f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1)) + 1.29f * cosf(4.0f * M_PI * i / (FFT_SIZE - 1)) - 0.388f * cosf(6.0f * M_PI * i / (FFT_SIZE - 1)) + 0.032f * cosf(8.0f * M_PI * i / (FFT_SIZE - 1));
                break;
            default:
                window = 1.0f;  // Default to rectangle
                break;
        }
        sample *= window;
        
        // Store as complex number (real part only, imaginary = 0)
        g_fft_analyzer.fft_input[i].r = sample;
        g_fft_analyzer.fft_input[i].i = 0.0f;
    }
    
    // Perform FFT using kiss_fft
    kiss_fft(g_fft_analyzer.fft_cfg, g_fft_analyzer.fft_input, g_fft_analyzer.fft_output);
    
    // Calculate magnitude spectrum (only positive frequencies)
    for (int i = 0; i < FFT_SIZE/2; i++) {
        float real = g_fft_analyzer.fft_output[i].r;
        float imag = g_fft_analyzer.fft_output[i].i;
        
        // Calculate magnitude
        float magnitude = sqrtf(real * real + imag * imag);
        
        // Normalize by FFT size
        magnitude /= FFT_SIZE;
        
        // Convert to dB scale (with protection against log(0))
        if (magnitude > 1e-10f) {
            g_fft_analyzer.magnitude[i] = 20.0f * log10f(magnitude);
        } else {
            g_fft_analyzer.magnitude[i] = -200.0f;  // Very low value
        }
    }
    
    // Prepare spectrum data for display (downsampled if needed)
    int bins_per_pixel = (FFT_SIZE/2) / FREQ_BINS;
    if (bins_per_pixel < 1) bins_per_pixel = 1;
    
    for (int i = 0; i < FREQ_BINS && i * bins_per_pixel < FFT_SIZE/2; i++) {
        int bin_index = i * bins_per_pixel;
        
        // Only include frequencies in our range of interest
        float freq = fft_bin_to_frequency(bin_index);
        if (freq >= MIN_FREQ && freq <= MAX_FREQ) {
            // Convert dB to display value (0-1000 range)
            float db_value = g_fft_analyzer.magnitude[bin_index];
            int display_value = (int)((db_value + 100.0f) * 10.0f);  // -100dB to 0dB -> 0 to 1000
            
            if (display_value < 0) display_value = 0;
            if (display_value > 1000) display_value = 1000;
            
            g_fft_analyzer.spectrum_data[i] = display_value;
        } else {
            g_fft_analyzer.spectrum_data[i] = 0;
        }
    }
    
    g_fft_analyzer.data_ready = true;
    
    printf("FFT processed - Peak at %.1f Hz: %.1f dB\n", 
           fft_bin_to_frequency(32), g_fft_analyzer.magnitude[32]);  // Sample bin for debug
}

/**
 * Convert FFT bin index to frequency in Hz
 */
float fft_bin_to_frequency(int bin) {
    return (float)bin * SAMPLE_RATE / (float)FFT_SIZE;
}
