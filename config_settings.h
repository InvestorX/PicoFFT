/*****************************************************************************
* | File      	:   config_settings.h
* | Author      :   FFT Spectrum Analyzer Configuration
* | Function    :   Central configuration definitions for all FFT settings
* | Info        :   All configurable parameters for the spectrum analyzer
******************************************************************************/
#ifndef __CONFIG_SETTINGS_H
#define __CONFIG_SETTINGS_H

#include <stdint.h>

// ========================================
// 🔧 中央設定ファイル - config_settings.h
// ========================================

// ** フレームレート設定 **
#define TARGET_FPS 30                                // 目標フレームレート（FPS）
#define TARGET_FRAME_TIME_US (1000000 / TARGET_FPS) // フレーム時間（マイクロ秒）

// ** ADCサンプリング設定 **
#define SAMPLING_RATE_HZ 128000                      // サンプリング周波数（128kHz）
#define SAMPLING_INTERVAL_US (1000000.0 / SAMPLING_RATE_HZ) // サンプリング間隔（μs）

// ** ADC電圧設定 **
#define ADC_REFERENCE_VOLTAGE 3.3f                  // ADC基準電圧（3.3V）
#define ADC_OFFSET_VOLTAGE 1.65f                    // ADCオフセット電圧（1.65V = Vref/2）
#define ADC_RESOLUTION_BITS 12                      // ADC分解能（12bit）
#define ADC_VOLTAGE_PER_BIT (ADC_REFERENCE_VOLTAGE / (1 << ADC_RESOLUTION_BITS)) // 1bitあたりの電圧

// ** dB基準電圧設定 **
#define DB_REFERENCE_VOLTAGE_0DBM 0.274f            // 0dBm基準電圧（実効値）- 75Ω系の正しい理論値
#define DB_REFERENCE_IMPEDANCE 75.0f                // 基準インピーダンス（75Ω）
// 0dBm電圧計算（正しい計算）:
// 75Ω系:  0dBm = 1mW @ 75Ω  = √(0.001W × 75Ω)  = 0.274Vrms
// 50Ω系:  0dBm = 1mW @ 50Ω  = √(0.001W × 50Ω)  = 0.224Vrms = 0.632Vp-p → Vp-p/2 = 0.316V
// 600Ω系: 0dBm = 1mW @ 600Ω = √(0.001W × 600Ω) = 0.775Vrms = 2.191Vp-p → Vp-p/2 = 1.095V

// ** インピーダンス設定 **
#define ADC_INPUT_IMPEDANCE 100000.0f               // ADC入力インピーダンス（100kΩ）
#define SIGNAL_SOURCE_IMPEDANCE 75.0f               // 信号源インピーダンス（75Ω）
#define IMPEDANCE_CORRECTION_FACTOR ((ADC_INPUT_IMPEDANCE + SIGNAL_SOURCE_IMPEDANCE) / ADC_INPUT_IMPEDANCE)

// ** 周波数スケール設定 **
#define USE_LOG_FREQ_SCALE 0                        // 1=対数スケール, 0=リニアスケール

// ** ピークホールド設定 **
#define PEAK_HOLD_DURATION_MS 1                     // ピークホールド時間（ミリ秒）

// ** FFT窓関数設定 **
// 窓関数タイプ選択: 0=レクタングル, 1=ハミング, 2=ハン, 3=ブラックマン, 4=ブラックマン・ハリス, 5=カイザー・ベッセル, 6=フラットトップ
#define FFT_WINDOW_TYPE 0                           // 使用する窓関数

// 各窓関数のコヒーレントゲイン補正係数（理論値）
#define WINDOW_COHERENT_GAIN_RECTANGLE 1.0f         // レクタングル窓（矩形窓）
#define WINDOW_COHERENT_GAIN_HAMMING 0.54f          // ハミング窓
#define WINDOW_COHERENT_GAIN_HANN 0.5f              // ハン窓（ハニング）
#define WINDOW_COHERENT_GAIN_BLACKMAN 0.42f         // ブラックマン窓
#define WINDOW_COHERENT_GAIN_BLACKMANHARRIS 0.35875f // ブラックマン・ハリス窓
#define WINDOW_COHERENT_GAIN_KAISER_BESSEL 0.4f     // カイザー・ベッセル窓（β=8.5）
#define WINDOW_COHERENT_GAIN_FLATTOP 0.2156f        // フラットトップ窓

// 各窓関数の振幅補正係数（1/コヒーレントゲイン）
#define WINDOW_AMPLITUDE_CORRECTION_RECTANGLE (1.0f / 1.0f)          // 1.0
#define WINDOW_AMPLITUDE_CORRECTION_HAMMING (1.0f / 0.54f)           // ≈1.8519
#define WINDOW_AMPLITUDE_CORRECTION_HANN (1.0f / 0.5f)               // 2.0
#define WINDOW_AMPLITUDE_CORRECTION_BLACKMAN (1.0f / 0.42f)          // ≈2.3810
#define WINDOW_AMPLITUDE_CORRECTION_BLACKMANHARRIS (1.0f / 0.35875f) // ≈2.7881
#define WINDOW_AMPLITUDE_CORRECTION_KAISER_BESSEL (1.0f / 0.4f)      // 2.5
#define WINDOW_AMPLITUDE_CORRECTION_FLATTOP (1.0f / 0.2156f)         // ≈4.6385

// カイザー・ベッセル窓パラメータ
#define KAISER_BESSEL_BETA 8.5f                     // カイザー・ベッセル窓のβパラメータ（高精度）

// ** ADCサンプリング設定 **
#define ADC_DMA_ENABLED 1                           // 0=手動サンプリング, 1=DMAサンプリング
#define ADC_BUFFER_COUNT 2                          // ダブルバッファリング用バッファ数
#define ADC_DMA_PRIORITY 0                          // DMA割り込み優先度 (0=最高)
#define ADC_DMA_CHANNEL_AUTO -1                     // -1=自動選択, 0-11=手動指定

// ** DMAサンプリング高度設定 **
#define ADC_DMA_TRANSFER_SIZE DMA_SIZE_16           // DMA転送サイズ (16bit ADC)
#define ADC_DMA_RING_BUFFER_MODE 1                  // 1=リングバッファ, 0=ワンショット
#define ADC_DMA_ERROR_RECOVERY 1                    // 1=エラー自動回復, 0=手動回復
#define ADC_DMA_OVERRUN_DETECTION 1                 // 1=オーバーラン検出有効, 0=無効

// ** 表示設定 **
#define FREQUENCY_RANGE_MIN 1000                    // 最低周波数（1kHz）
#define FREQUENCY_RANGE_MAX 50000                   // 最高周波数（50kHz）
#define AMPLITUDE_RANGE_MIN_DB -100                 // 最小振幅（dB）
#define AMPLITUDE_RANGE_MAX_DB 20                   // 最大振幅（dB）

// ** 表示座標補正設定 **
#define FREQUENCY_DISPLAY_OFFSET_HZ -2500           // 周波数表示オフセット（Hzで指定）- 負値で左にシフト ADC_DMA_ENABLEDを手動にするときだけ、オフセット入れる
#define ENABLE_FREQUENCY_OFFSET_CORRECTION 0        // 1=オフセット補正有効, 0=無効

// ** 周波数マーカー設定 **
// 1kHz, 5kHz, 10kHz, 15kHz, 20kHz, 25kHz, 30kHz, 35kHz, 40kHz, 45kHz, 50kHz (5kHz刻み)
#define FREQ_MARKERS_COUNT 11                       // マーカー数
#define FREQ_MARKERS_HZ_ARRAY {1000, 5000, 10000, 15000, 20000, 25000, 30000, 35000, 40000, 45000, 50000}

#endif
