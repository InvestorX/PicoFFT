
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "config_settings.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "DEV_Config.h"
#include "LCD_Driver.h"
#include "fft_streaming_display.h"
#include "main.h"   //Examples

// ========================================
// 🔧 グローバル変数定義 - config_settings.hの設定を実体化
// ========================================

// 周波数マーカー配列の実体定義（config_settings.hで宣言、main.hで外部宣言）
const uint32_t FREQ_MARKERS_HZ[FREQ_MARKERS_COUNT] = FREQ_MARKERS_HZ_ARRAY;

int main() {
    // USB/UART stdio初期化 - デバッグ出力用
    stdio_init_all();
    
    // システム情報表示 - 設定値を表示
    printf("Pico-ResTouch-LCD FFT Spectrum Analyzer - Configurable Edition\n");
    printf("Frame Rate: %dFPS (Target: %d μs/frame)\n", TARGET_FPS, TARGET_FRAME_TIME_US);
    printf("Frequency Range: %d-%dkHz (%s Scale, 5kHz steps)\n", 
            FREQUENCY_RANGE_MIN/1000, FREQUENCY_RANGE_MAX/1000,
            USE_LOG_FREQ_SCALE ? "Log" : "Linear");
    printf("Frequency Markers: %d points (1k-50k in 5kHz steps)\n", FREQ_MARKERS_COUNT);
    printf("Amplitude Range: %d to %ddB\n", AMPLITUDE_RANGE_MIN_DB, AMPLITUDE_RANGE_MAX_DB);
    printf("Sampling Rate: %dkHz (Interval: %.3f μs)\n", 
            SAMPLING_RATE_HZ/1000, SAMPLING_INTERVAL_US);

    // 窓関数設定情報
    const char* window_names[] = {"Rectangle", "Hamming", "Hann", "Blackman", "Blackman-Harris", "Kaiser-Bessel", "Flat-Top"};
    const float window_corrections[] = {
        WINDOW_AMPLITUDE_CORRECTION_RECTANGLE,
        WINDOW_AMPLITUDE_CORRECTION_HAMMING,
        WINDOW_AMPLITUDE_CORRECTION_HANN,
        WINDOW_AMPLITUDE_CORRECTION_BLACKMAN,
        WINDOW_AMPLITUDE_CORRECTION_BLACKMANHARRIS,
        WINDOW_AMPLITUDE_CORRECTION_KAISER_BESSEL,
        WINDOW_AMPLITUDE_CORRECTION_FLATTOP
    };
    printf("FFT Window: %s (Type=%d, Correction=%.4f)\n", 
            window_names[FFT_WINDOW_TYPE], FFT_WINDOW_TYPE, window_corrections[FFT_WINDOW_TYPE]);

    printf("ADC Settings: Vref=%.2fV, Offset=%.2fV, Resolution=%d-bit (%.3fmV/bit)\n",
            ADC_REFERENCE_VOLTAGE, ADC_OFFSET_VOLTAGE, ADC_RESOLUTION_BITS, ADC_VOLTAGE_PER_BIT*1000);
    printf("dB Reference: 0dBm = %.3fV (Vp-p/2), Impedance = %.0fΩ\n",
            DB_REFERENCE_VOLTAGE_0DBM, DB_REFERENCE_IMPEDANCE);
    printf("ADC Input: Zin = %.0fkΩ, Source = %.0fΩ, Correction = %.5f\n",
            ADC_INPUT_IMPEDANCE/1000, SIGNAL_SOURCE_IMPEDANCE, IMPEDANCE_CORRECTION_FACTOR);
    printf("Peak Hold: %.1f seconds\n", PEAK_HOLD_DURATION_MS/1000.0);
    printf("Display: Green=Current Spectrum, Cyan=Peak Hold\n");
    printf("Starting FFT analysis with centralized configuration...\n");
    
    // リアルタイムFFT解析開始 - メイン処理ループ
    // この関数は無限ループで設定可能FPSでのスペクトラム表示を実行
    fft_realtime_analysis();
    
    return 0;
}
