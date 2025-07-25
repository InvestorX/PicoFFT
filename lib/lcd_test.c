/*****************************************************************************
* | File      	:   lcd_2in8.c
* | Author      :   Waveshare team
* | Function    :   2.9inch e-paper V2 test demo
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2021-01-20
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
******************************************************************************/
//#include "main.h"
#include "config_settings.h"
#include "LCD_Driver.h"
#include "LCD_Touch.h"
#include "LCD_GUI.h"
#include "LCD_Bmp.h"
#include "DEV_Config.h"
#include <stdio.h>
#include <math.h>
#include "hardware/watchdog.h"
#include "pico/stdlib.h"
#include "fft_streaming_display.h"
#include "fft_analyzer.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "kiss_fft.h"

/**
 * LCD機能テスト関数
 * 
 * 機能: LCD画面とタッチパネルの基本動作をテストする
 * 引数: なし
 * 戻り値: int - 正常終了時は0、エラー時は負の値
 * 
 * 処理内容:
 * - システム初期化
 * - LCD/タッチパネル初期化
 * - GUI表示とBMP画像表示
 * - タッチパネルキャリブレーション
 * - 描画ボード表示（無限ループ）
 */
int lcd_test(void)
{
	uint8_t counter = 0;
    
	System_Init();
	SD_Init();
	LCD_SCAN_DIR  lcd_scan_dir = D2U_L2R;  // 90° rotation for landscape mode
	LCD_Init(lcd_scan_dir,800);
	TP_Init(lcd_scan_dir);

	GUI_Show();
	Driver_Delay_ms(1000);

	LCD_Show_bmp(lcd_scan_dir);
	Driver_Delay_ms(2000);

	TP_GetAdFac();
	TP_Dialog(lcd_scan_dir);
	while(1){
		TP_DrawBoard(lcd_scan_dir);
	}

	return 0;
}

/**
 * FFT軸表示テスト関数
 * 
 * 機能: FFTスペクトラム表示の軸ラベルのみを表示してテストする
 * 引数: なし
 * 戻り値: int - 正常終了時は0、エラー時は負の値
 * 
 * 処理内容:
 * - システム/LCD/ストリーミング表示システムの初期化
 * - 固定軸ラベルの表示（周波数軸: 100Hz-50kHz, 振幅軸: -100dB to +20dB）
 * - 軸表示の動作確認（無限待機）
 * 
 * 注意: この関数は軸表示のデバッグ用途のみ
 */
int fft_axis_test(void) {
    printf("=== FFT Axis Test Started ===\n");
    
    // Initialize system
    stdio_init_all();
    sleep_ms(1000);  // Stabilization delay
    
    printf("Initializing system...\n");
    System_Init();
    
    printf("Initializing LCD for landscape mode...\n");
    // Initialize LCD in landscape mode (320x240) - D2U_L2R is 90° rotation for landscape
    LCD_Init(D2U_L2R, 100);  // Down to up, left to right, 100% backlight
    LCD_Clear(BLACK);
    printf("LCD initialized.\n");
    
    printf("Initializing streaming display system...\n");
    // Initialize streaming display system
    fft_streaming_display_init();
    printf("Streaming display system initialized.\n");
    
    // TEST: Display only axes for debugging
    printf("Testing fixed axis labels only...\n");
    fft_streaming_display_test_axes_only();
    printf("Axis test function called.\n");
    
    printf("Axis test complete. The program will wait here.\n");
    printf("Check the LCD display for axis labels.\n");
    printf("Frequency axis: 100Hz, 1k, 5k, 20k, 50k\n");
    printf("Amplitude axis: -100, -60, -20, 0, +20 dBm\n");
    
    // Wait indefinitely to observe the axis display
    while (true) {
        sleep_ms(1000);
    }
    
    return 0;
}

/**
 * リアルタイムADC FFT解析メイン関数
 * 
 * 機能: GP26ピンからのADC入力をリアルタイムでFFT解析し、スペクトラムを表示する
 * 引数: なし
 * 戻り値: int - 正常終了時は0、エラー時は負の値
 * 
 * システム仕様:
 * - サンプリング周波数: 128kHz（厳密）
 * - FFTサイズ: 1024サンプル
 * - 周波数範囲: 100Hz - 50kHz
 * - 振幅範囲: -100dB to +20dB（設計要件）
 * - フレームレート: 30FPS（リアルタイム表示）
 * - ADC基準: ハーフスケール（2048）= 0dB
 * 
 * 処理フロー:
 * 1. システム/LCD/ADC/FFTアナライザーの初期化
 * 2. メインループ（無限）:
 *    - ADCサンプリング（128kHz、1024サンプル）
 *    - DC除去とハミング窓関数適用
 *    - kiss_fftによるFFT実行
 *    - 振幅スペクトラム計算（dB変換）
 *    - ストリーミング表示更新
 *    - デバッグ出力（電圧監視、周波数解析）
 *    - フレームレート制御（30FPS）
 * 
 * 1kHz信号検出:
 * - 期待ビン: bin 8 (1000Hz = 8 × 128000 / 1024)
 * - 周波数分解能: 125Hz/bin
 */
int fft_realtime_analysis(void) {
    static bool initialized = false;
    static float* magnitude_db = NULL;
    
    if (!initialized) {
        printf("=== Real-time ADC FFT Analysis Started ===\n");
        
        printf("Initializing system...\n");
        System_Init();
        
        printf("Initializing LCD for landscape mode...\n");
        // Initialize LCD in landscape mode (320x240)
        LCD_Init(D2U_L2R, 100);  // Down to up, left to right, 100% backlight
        LCD_Clear(BLACK);
        printf("LCD initialized.\n");
        
        printf("Initializing streaming display system...\n");
        // Initialize streaming display system
        fft_streaming_display_init();
        printf("Streaming display system initialized.\n");
        
        printf("Initializing ADC system (Vref=%.2fV, %d-bit)...\n", 
               ADC_REFERENCE_VOLTAGE, ADC_RESOLUTION_BITS);
        // Initialize ADC for precise 128kHz sampling to avoid aliasing
        adc_init();
        adc_gpio_init(26);  // GP26 as ADC input
        adc_select_input(0); // Select ADC0 (GP26)
        
        // Calculate precise ADC clock divider for 128kHz sampling (main.cで設定)
        // Pico 2W ADC clock = 48MHz
        // Target sampling rate = main.c SAMPLING_RATE_HZ
        // Clock divider = 48MHz / SAMPLING_RATE_HZ
        float target_sample_rate = (float)SAMPLING_RATE_HZ;  // main.cから取得
        float adc_clock_hz = 48000000.0f;       // 48MHz
        float clk_div = adc_clock_hz / target_sample_rate;
        adc_set_clkdiv(clk_div);
        
        printf("ADC initialized on GP26 with clk_div=%.1f for %.0fHz sampling rate.\n", 
               clk_div, target_sample_rate);
        
        printf("Initializing FFT analyzer...\n");
        // Initialize FFT analyzer
        // FFTアナライザー初期化 - kiss_fftライブラリ使用
        fft_analyzer_init();
        printf("FFT analyzer initialized.\n");
        
        // 振幅スペクトラム表示用メモリ確保（FFTサイズの半分 = 正の周波数のみ）
        // Allocate magnitude buffer for display (half FFT size = positive frequencies only)
        magnitude_db = (float*)malloc(sizeof(float) * FFT_SIZE / 2);
        if (!magnitude_db) {
            printf("ERROR: Failed to allocate FFT buffer\n");
            return -1;
        }
        
        printf("Starting ultra-high-speed real-time FFT analysis (60FPS)...\n");
        printf("ADC Input: GP26 (12-bit, 0-3.3V)\n");
        printf("Sample Rate: 125kHz (Precisely Timed)\n");
        printf("FFT Size: %d samples (1024)\n", FFT_SIZE);
        printf("Frequency Range: 100Hz - 50kHz\n");
        printf("Frame Rate: 60FPS (Ultra High Speed)\n");
        printf("Expected 100Hz at bin 1 (122Hz), 1kHz at bin 8 (1000Hz)\n");
        printf("Press Ctrl+C to stop.\n");
        
        initialized = true;
    }
    
    static int frame_count = 0;
    static absolute_time_t start_time;
    static bool first_run = true;
    
    if (first_run) {
        start_time = get_absolute_time();
        first_run = false;
    }
    
    // サンプリングレート実測用変数
    static float measured_sample_rate = 128000.0f;  // 初期値は理論値
    static bool rate_calibrated = false;
    
    // Voltage monitoring variables
    float voltage_sum = 0.0f;
    float voltage_min = 3.3f;
    float voltage_max = 0.0f;
    int voltage_sample_count = 0;
    
    // Main analysis loop - 60FPS対応（さらに高速化）
    while (true) {
        absolute_time_t frame_start = get_absolute_time();
        
        // Sample ADC data directly into FFT buffer with rate measurement
        voltage_sum = 0.0f;
        voltage_min = 3.3f;
        voltage_max = 0.0f;
        voltage_sample_count = FFT_SIZE;
        
        // サンプリング時間測定開始 - 実際のレートを測定
        absolute_time_t sampling_start = get_absolute_time();
        
        for (int i = 0; i < FFT_SIZE; i++) {
            uint16_t adc_raw = adc_read();
            
            // Convert 12-bit ADC to voltage using main.c settings
            float voltage = (float)adc_raw * ADC_VOLTAGE_PER_BIT;
            voltage_sum += voltage;
            if (voltage < voltage_min) voltage_min = voltage;
            if (voltage > voltage_max) voltage_max = voltage;
            
            // Store ADC sample directly in FFT buffer
            g_fft_analyzer.adc_buffer[i] = adc_raw;
            
            // Precise timing for 128kHz sampling (7.8125us per sample)
            // Use busy-wait for precise timing control
            sleep_us(SAMPLING_INTERVAL_US);  // main.cで定義された128kHz厳密タイミング
        }
        
        // サンプリング時間測定終了 - 実際のサンプリングレートを計算
        absolute_time_t sampling_end = get_absolute_time();
        float sampling_time_us = (float)absolute_time_diff_us(sampling_start, sampling_end);
        float current_sample_rate = (float)FFT_SIZE * 1000000.0f / sampling_time_us;
        
        // 初回測定またはキャリブレーション（最初の10フレームで平均化）
        if (!rate_calibrated && frame_count < 10) {
            measured_sample_rate = (measured_sample_rate * frame_count + current_sample_rate) / (frame_count + 1);
            if (frame_count == 9) {
                rate_calibrated = true;
                printf("Calibrated sample rate: %.1f Hz (theory: 128000 Hz)\n", measured_sample_rate);
                printf("Rate difference: %.1f%% (bin shift compensation applied)\n", 
                       (measured_sample_rate - 128000.0f) / 128000.0f * 100.0f);
            }
        }
        
        // 実測サンプリングレートを使用 - bin 7問題の解決
        const float sample_rate = measured_sample_rate;
        float freq_resolution = sample_rate / (float)FFT_SIZE;
        
        // Process FFT manually since we already have the data
        // Calculate DC offset for removal (使用main.cの設定値)
        float dc_offset = 0;
        for (int i = 0; i < FFT_SIZE; i++) {
            dc_offset += g_fft_analyzer.adc_buffer[i];
        }
        dc_offset /= FFT_SIZE;
        
        // Convert DC offset to voltage for monitoring
        float dc_offset_voltage = dc_offset * ADC_VOLTAGE_PER_BIT;
        
        // Prepare FFT input with configurable windowing and DC removal
        for (int i = 0; i < FFT_SIZE; i++) {
            // Remove DC offset and convert to float
            float sample = (float)(g_fft_analyzer.adc_buffer[i]) - dc_offset;
            
            // Apply selected window function
            float window;
            switch (FFT_WINDOW_TYPE) {
                case 0: // Hamming window
                    window = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1));
                    break;
                case 1: // Hann window (Hanning)
                    window = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (FFT_SIZE - 1)));
                    break;
                case 2: // Blackman window
                    window = 0.42f - 0.5f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1)) + 
                             0.08f * cosf(4.0f * M_PI * i / (FFT_SIZE - 1));
                    break;
                case 3: // Blackman-Harris window
                    window = 0.35875f - 0.48829f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1)) + 
                             0.14128f * cosf(4.0f * M_PI * i / (FFT_SIZE - 1)) - 
                             0.01168f * cosf(6.0f * M_PI * i / (FFT_SIZE - 1));
                    break;
                case 4: // Kaiser-Bessel window (β=8.5)
                    {
                        // Kaiser-Bessel window calculation with β = 8.5
                        float alpha = (float)(FFT_SIZE - 1) / 2.0f;
                        float n_norm = (i - alpha) / alpha;  // Normalize to [-1, 1]
                        
                        // Modified Bessel function of first kind I0 approximation
                        float x = KAISER_BESSEL_BETA * sqrtf(1.0f - n_norm * n_norm);
                        float i0_x = 1.0f;
                        float term = 1.0f;
                        for (int k = 1; k <= 10; k++) {  // 10 terms for good accuracy
                            term *= (x / (2.0f * k)) * (x / (2.0f * k));
                            i0_x += term;
                        }
                        
                        // I0(β) calculation (β = 8.5)
                        float i0_beta = 1.0f;
                        term = 1.0f;
                        for (int k = 1; k <= 10; k++) {
                            term *= (KAISER_BESSEL_BETA / (2.0f * k)) * (KAISER_BESSEL_BETA / (2.0f * k));
                            i0_beta += term;
                        }
                        
                        window = i0_x / i0_beta;
                    }
                    break;
                case 5: // Flat-top window
                    {
                        float n_norm = 2.0f * M_PI * i / (FFT_SIZE - 1);
                        window = 0.21557895f - 0.41663158f * cosf(n_norm) + 
                                 0.277263158f * cosf(2.0f * n_norm) - 
                                 0.083578947f * cosf(3.0f * n_norm) + 
                                 0.006947368f * cosf(4.0f * n_norm);
                    }
                    break;
                default: // Fallback to Hamming
                    window = 0.54f - 0.46f * cosf(2.0f * M_PI * i / (FFT_SIZE - 1));
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
        // Apply appropriate window correction factor based on selected window
        float window_correction;
        switch (FFT_WINDOW_TYPE) {
            case 0: // Hamming
                window_correction = WINDOW_AMPLITUDE_CORRECTION_HAMMING;
                break;
            case 1: // Hann
                window_correction = WINDOW_AMPLITUDE_CORRECTION_HANN;
                break;
            case 2: // Blackman
                window_correction = WINDOW_AMPLITUDE_CORRECTION_BLACKMAN;
                break;
            case 3: // Blackman-Harris
                window_correction = WINDOW_AMPLITUDE_CORRECTION_BLACKMANHARRIS;
                break;
            case 4: // Kaiser-Bessel
                window_correction = WINDOW_AMPLITUDE_CORRECTION_KAISER_BESSEL;
                break;
            case 5: // Flat-top
                window_correction = WINDOW_AMPLITUDE_CORRECTION_FLATTOP;
                break;
            default: // Fallback to Hamming
                window_correction = WINDOW_AMPLITUDE_CORRECTION_HAMMING;
                break;
        }
        
        for (int i = 0; i < FFT_SIZE/2; i++) {
            float real = g_fft_analyzer.fft_output[i].r;
            float imag = g_fft_analyzer.fft_output[i].i;
            
            // Calculate magnitude
            float magnitude = sqrtf(real * real + imag * imag);
            
            // Apply window correction
            magnitude *= window_correction;
            
            // Improved normalization for kiss_fft
            // For real input signals, divide by FFT_SIZE/2 (except DC and Nyquist)
            if (i == 0) {
                magnitude /= FFT_SIZE;  // DC component
            } else {
                magnitude /= (FFT_SIZE / 2);  // Other frequencies (compensate for real signal symmetry)
            }
            
            // Convert to dB scale with main.cで定義された0dBm基準電圧を使用
            // magnitude は ADC カウント値なので、まず電圧に変換してからdB計算
            float voltage_magnitude = magnitude * ADC_VOLTAGE_PER_BIT;  // ADCカウント → 電圧
            
            // インピーダンスマッチング補正を適用
            // ADC入力インピーダンス（高インピーダンス）と信号源インピーダンスの分圧を補正
            float corrected_voltage = voltage_magnitude * IMPEDANCE_CORRECTION_FACTOR;
            
            // Vp-p/2 を基準とするdB計算（main.c DB_REFERENCE_VOLTAGE_0DBM が 0dBm基準）
            float db_value;
            if (corrected_voltage > 1e-9f) {  // 1nV以上の信号
                db_value = 20.0f * log10f(corrected_voltage / DB_REFERENCE_VOLTAGE_0DBM);
            } else {
                db_value = -120.0f;  // Very low value for near-zero signals
            }
            
            g_fft_analyzer.magnitude[i] = db_value;
        }
        
        g_fft_analyzer.data_ready = true;
        
        // Use the magnitude spectrum calculated by kiss_fft FFT analyzer
        for (int i = 0; i < FFT_SIZE / 2; i++) {
            // kiss_fft analyzer calculates magnitude in dB
            magnitude_db[i] = g_fft_analyzer.magnitude[i];
            
            // Apply design requirement amplitude limits (-100 to +20dB)
            if (magnitude_db[i] > 20.0f) magnitude_db[i] = 20.0f;     // Design requirement upper limit
            if (magnitude_db[i] < -100.0f) magnitude_db[i] = -100.0f; // Design requirement lower limit
        }
        
        // Find peak for frequency analysis
        float max_magnitude = -200.0f;
        int max_bin = 1;  // Skip DC component (bin 0 = 0Hz)
        
        for (int i = 1; i < FFT_SIZE / 2; i++) { // Skip DC bin
            if (magnitude_db[i] > max_magnitude) {
                max_magnitude = magnitude_db[i];
                max_bin = i;
            }
        }
        
        // Debug: Show expected vs actual 1kHz detection with detailed sampling info
        int expected_1khz_bin = (int)(1000.0f * FFT_SIZE / sample_rate);
        float voltage_avg = voltage_sum / (float)voltage_sample_count;
        float voltage_pp = voltage_max - voltage_min;
        
        printf("Sample Rate: %.1fHz (theory: 128000Hz, diff: %.1f%%) | ", 
               sample_rate, (sample_rate - 128000.0f) / 128000.0f * 100.0f);
        printf("ADC: Avg=%.2fV, P-P=%.2fV | ", voltage_avg, voltage_pp);
        printf("1kHz Expected: bin %d (%.1fHz), Peak: bin %d (%.1fHz) at %.1fdB\n", 
               expected_1khz_bin, (float)expected_1khz_bin * sample_rate / FFT_SIZE,
               max_bin, (float)max_bin * sample_rate / FFT_SIZE, max_magnitude);
        
        // Show magnitude around expected 1kHz bin with raw FFT values
        printf("Bins around 1kHz: ");
        for (int i = expected_1khz_bin - 2; i <= expected_1khz_bin + 2; i++) {
            if (i >= 0 && i < FFT_SIZE/2) {
                float raw_mag = sqrtf(g_fft_analyzer.fft_output[i].r * g_fft_analyzer.fft_output[i].r + 
                                     g_fft_analyzer.fft_output[i].i * g_fft_analyzer.fft_output[i].i);
                printf("bin%d(%.0fHz):%.1fdB(raw=%.1f) ", i, (float)i * sample_rate / FFT_SIZE, 
                       magnitude_db[i], raw_mag);
            }
        }
        printf("\n");
        
        // Update streaming display
        fft_streaming_display_update_spectrum(magnitude_db, sample_rate);
        
        frame_count++;
        
        // リアルタイム電圧監視表示 - FPS設定に応じて頻度調整
        // Real-time voltage monitoring - frequency adjusted for FPS setting
        // 1 FPS: 10フレーム = 10秒間隔
        // 60 FPS: 600フレーム = 10秒間隔
        if (frame_count % 10 == 0) {  // 1 FPSでは10秒間隔
            float voltage_avg = voltage_sum / (float)voltage_sample_count;
            float voltage_pp = voltage_max - voltage_min;
            
            // Calculate peak frequency using measured sample rate
            float peak_freq = (float)max_bin * sample_rate / (float)FFT_SIZE;
            float peak_db = max_magnitude;  // Already in dB
            
            printf("Input: %.3fV avg, %.3fV p-p | Peak: %.1fHz at %.1fdB (Rate: %.1fHz)\n", 
                   voltage_avg, voltage_pp, peak_freq, peak_db, sample_rate);
        }
        
        // パフォーマンス監視表示 - 60FPS対応で頻度調整（約3秒間隔）
        // Performance monitoring - adjusted for current FPS setting
        // パフォーマンス監視 - 現在のFPS設定に合わせて調整
        // 1 FPS: 10フレーム = 10秒間隔
        // 60 FPS: 180フレーム = 3秒間隔
        if (frame_count % 10 == 0) {  // 1 FPSでは10秒間隔
            absolute_time_t current_time = get_absolute_time();
            int64_t elapsed_us = absolute_time_diff_us(start_time, current_time);
            float elapsed_seconds = (float)elapsed_us / 1000000.0f;
            float fps = (float)frame_count / elapsed_seconds;
            
            // Calculate voltage statistics
            float voltage_avg = voltage_sum / (float)voltage_sample_count;
            float voltage_pp = voltage_max - voltage_min; // Peak-to-peak
            
            printf("FFT Analysis: %.1fs - FPS: %.2f, Frames: %d\n", 
                   elapsed_seconds, fps, frame_count);
            printf("Voltage - Avg: %.3fV, Min: %.3fV, Max: %.3fV, P-P: %.3fV\n",
                   voltage_avg, voltage_min, voltage_max, voltage_pp);
        }
        
        // Frame rate limiting - adjustable FPS for different measurement needs
        // フレームレート制限 - 測定ニーズに応じて調整可能
        absolute_time_t frame_end = get_absolute_time();
        int64_t frame_time_us = absolute_time_diff_us(frame_start, frame_end);
        
        // FPS Settings (マイクロ秒単位) - main.cで設定可能:
        // 1 FPS   = 1000000us (1秒)    - 低速測定、省電力
        // 5 FPS   = 200000us (200ms)   - 省電力測定
        // 10 FPS  = 100000us (100ms)   - 基本測定
        // 30 FPS  = 33333us (33.33ms)  - 標準高速測定
        // 60 FPS  = 16667us (16.67ms)  - 超高速測定
        int64_t target_frame_time_us = TARGET_FRAME_TIME_US; // main.cで定義されたフレーム間隔
        
        if (frame_time_us < target_frame_time_us) {
            sleep_us(target_frame_time_us - frame_time_us);
        }
    }
    
    // This point is never reached due to infinite loop
    // But added for proper function structure
    free(magnitude_db);
    return 0;
}
