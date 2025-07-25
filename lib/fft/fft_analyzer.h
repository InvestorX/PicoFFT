/*****************************************************************************
* | File      	:   fft_analyzer.h
* | Author      :   
* | Function    :   FFT spectrum analyzer for Raspberry Pi Pico 2W using kiss_fft
* | Info        :   
*   - ADC sampling at 128kHz on GP26
*   - FFT analysis using kiss_fft library
*   - Core FFT functionality without display dependencies
*----------------
******************************************************************************/

#ifndef __FFT_ANALYZER_H
#define __FFT_ANALYZER_H

// Enable math constants in math.h
#define _USE_MATH_DEFINES

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "kiss_fft.h"

// Math constants - ensure M_PI is defined
#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// FFT Configuration - main.cで定義された設定値を使用
#define FFT_SIZE 1024           // FFT sample size (must be power of 2)
#define SAMPLE_RATE 128000      // 128kHz sampling rate (main.c SAMPLING_RATE_HZ)
#define ADC_CHANNEL 0           // GP26 is ADC0
#define DMA_CHANNEL 0

// Frequency range configuration - main.cで定義された値と対応
#define MIN_FREQ 1000           // 1kHz minimum frequency (main.c FREQUENCY_RANGE_MIN)
#define MAX_FREQ 50000          // 50kHz maximum frequency (main.c FREQUENCY_RANGE_MAX)
#define FREQ_BINS 512           // Number of frequency bins to analyze

// Data structures
typedef struct {
    uint16_t adc_buffer[FFT_SIZE];
    kiss_fft_cpx fft_input[FFT_SIZE];
    kiss_fft_cpx fft_output[FFT_SIZE];
    kiss_fft_cfg fft_cfg;
    float magnitude[FFT_SIZE/2];
    float freq_bins[FFT_SIZE/2];
    uint16_t spectrum_data[FREQ_BINS];
    bool data_ready;
} fft_analyzer_t;

// Function prototypes
void fft_analyzer_init(void);
void fft_analyzer_start_sampling(void);
void fft_analyzer_stop_sampling(void);
void fft_process_data(void);
float fft_bin_to_frequency(int bin);

// Global variable
extern fft_analyzer_t g_fft_analyzer;

#endif // __FFT_ANALYZER_H
