/**
 * fft_streaming_display.c - FFT Streaming Display Implementation
 * 
 * Features:
 * - Real-time spectrum streaming with optimized updates
 * - Fixed axis labels with logarithmic frequency scale (100Hz-50kHz)
 * - Linear dB amplitude scale (-100dBm to +20dBm)
 * - Bright white axis labels for high visibility
 * - Anti-flashing optimized display buffer
 */

#include "fft_streaming_display.h"
#include "config_settings.h"  // 設定定数
#include "main.h"  // main.cで定義された配列にアクセス
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "pico/time.h"

// Internal buffer for streaming display
static SpectrumPoint spectrum_buffer[STREAM_BUFFER_COLS];
static SpectrumHold hold_buffer[STREAM_BUFFER_COLS];  // Peak hold buffer for 0.5s hold
static bool buffer_initialized = false;

/**
 * Initialize streaming display system
 * 
 * 機能: FFTストリーミング表示システムの初期化
 * - スペクトラムバッファとピークホールドバッファの初期化
 * - 2秒間のピークホールド機能の準備
 * - LCD画面のクリアと軸の描画
 * 
 * 引数: なし
 * 戻り値: なし
 */
void fft_streaming_display_init(void) {
    memset(spectrum_buffer, 0, sizeof(spectrum_buffer));
    memset(hold_buffer, 0, sizeof(hold_buffer));
    buffer_initialized = true;
    
    // Initialize hold buffer with very low values for 2-second peak hold
    // ピークホールドバッファを低い値で初期化（2秒間保持用）
    for (int i = 0; i < STREAM_BUFFER_COLS; i++) {
        hold_buffer[i].peak_db = -200.0f;  // Very low initial value (background level)
        hold_buffer[i].hold_time = get_absolute_time(); // Set current time as baseline
    }
    
    fft_streaming_display_clear();
    fft_streaming_display_draw_axes();
}

/**
 * Clear entire spectrum display area
 */
void fft_streaming_display_clear(void) {
    GUI_DrawRectangle(STREAM_SPECTRUM_X, STREAM_SPECTRUM_Y, 
                        STREAM_SPECTRUM_X + STREAM_SPECTRUM_W + 50, 
                        STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H + 50, 
                        STREAM_COLOR_BG, DRAW_FULL, DOT_PIXEL_1X1);
}

/**
 * Convert frequency to normalized position (0.0 to 1.0)
 * Supports both linear and logarithmic scaling
 */
float fft_streaming_display_freq_to_position(float freq_hz) {
    if (freq_hz < FREQUENCY_RANGE_MIN) return 0.0f;
    if (freq_hz > FREQUENCY_RANGE_MAX) return 1.0f;
    
    if (USE_LOG_FREQ_SCALE) {
        // Logarithmic scale (main.cで設定)
        float log_freq = log10f(freq_hz);
        float log_min = log10f(FREQUENCY_RANGE_MIN);
        float log_max = log10f(FREQUENCY_RANGE_MAX);
        return (log_freq - log_min) / (log_max - log_min);
    } else {
        // Linear scale (main.cで設定)
        return (freq_hz - FREQUENCY_RANGE_MIN) / (FREQUENCY_RANGE_MAX - FREQUENCY_RANGE_MIN);
    }
}

/**
 * Convert frequency to display column
 */
int fft_streaming_display_freq_to_column(float freq_hz) {
    float normalized = fft_streaming_display_freq_to_position(freq_hz);
    int col = (int)(normalized * STREAM_BUFFER_COLS);
    if (col < 0) col = 0;
    if (col >= STREAM_BUFFER_COLS) col = STREAM_BUFFER_COLS - 1;
    return col;
}

/**
 * Simple digit patterns for axis labels - made larger and thicker
 */
static void draw_digit_0(int x, int y) {
    // 0 pattern (4x6) - larger and thicker
    for (int i = 0; i < 3; i++) {
        LCD_SetPointlColor(x+1+i, y+0, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+1+i, y+5, STREAM_COLOR_AXIS);
    }
    for (int i = 1; i < 5; i++) {
        LCD_SetPointlColor(x+0, y+i, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+3, y+i, STREAM_COLOR_AXIS);
    }
}

static void draw_digit_1(int x, int y) {
    // 1 pattern (4x6) - larger and thicker  
    for (int i = 0; i < 6; i++) {
        LCD_SetPointlColor(x+1, y+i, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+2, y+i, STREAM_COLOR_AXIS);
    }
    LCD_SetPointlColor(x+0, y+1, STREAM_COLOR_AXIS);
}

static void draw_digit_2(int x, int y) {
    // 2 pattern (4x6) - larger and thicker
    for (int i = 0; i < 4; i++) {
        LCD_SetPointlColor(x+i, y+0, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+i, y+3, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+i, y+5, STREAM_COLOR_AXIS);
    }
    LCD_SetPointlColor(x+3, y+1, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+3, y+2, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+0, y+4, STREAM_COLOR_AXIS);
}

static void draw_digit_5(int x, int y) {
    // 5 pattern (4x6) - larger and thicker
    for (int i = 0; i < 4; i++) {
        LCD_SetPointlColor(x+i, y+0, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+i, y+2, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+i, y+5, STREAM_COLOR_AXIS);
    }
    LCD_SetPointlColor(x+0, y+1, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+3, y+3, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+3, y+4, STREAM_COLOR_AXIS);
}

static void draw_digit_6(int x, int y) {
    // 6 pattern (4x6) - larger and thicker
    for (int i = 1; i < 4; i++) {
        LCD_SetPointlColor(x+i, y+0, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+i, y+3, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+i, y+5, STREAM_COLOR_AXIS);
    }
    for (int i = 1; i < 5; i++) {
        LCD_SetPointlColor(x+0, y+i, STREAM_COLOR_AXIS);
    }
    LCD_SetPointlColor(x+3, y+4, STREAM_COLOR_AXIS);
}

static void draw_digit_3(int x, int y) {
    // 3 pattern (4x6) - larger and thicker
    for (int i = 0; i < 4; i++) {
        LCD_SetPointlColor(x+i, y+0, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+i, y+3, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+i, y+5, STREAM_COLOR_AXIS);
    }
    LCD_SetPointlColor(x+3, y+1, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+3, y+2, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+3, y+4, STREAM_COLOR_AXIS);
}

static void draw_digit_7(int x, int y) {
    // 7 pattern (4x6) - larger and thicker
    for (int i = 0; i < 4; i++) {
        LCD_SetPointlColor(x+i, y+0, STREAM_COLOR_AXIS);
    }
    for (int i = 1; i < 6; i++) {
        LCD_SetPointlColor(x+3, y+i, STREAM_COLOR_AXIS);
    }
}

static void draw_digit_4(int x, int y) {
    // 4 pattern (4x6) - larger and thicker
    for (int i = 0; i < 3; i++) {
        LCD_SetPointlColor(x+0, y+i, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+3, y+i, STREAM_COLOR_AXIS);
    }
    for (int i = 0; i < 4; i++) {
        LCD_SetPointlColor(x+i, y+2, STREAM_COLOR_AXIS);
    }
    for (int i = 3; i < 6; i++) {
        LCD_SetPointlColor(x+3, y+i, STREAM_COLOR_AXIS);
    }
}

static void draw_digit_8(int x, int y) {
    // 8 pattern (4x6) - larger and thicker
    for (int i = 0; i < 4; i++) {
        LCD_SetPointlColor(x+i, y+0, STREAM_COLOR_AXIS); // top
        LCD_SetPointlColor(x+i, y+2, STREAM_COLOR_AXIS); // middle
        LCD_SetPointlColor(x+i, y+5, STREAM_COLOR_AXIS); // bottom
    }
    for (int i = 0; i < 6; i++) {
        LCD_SetPointlColor(x+0, y+i, STREAM_COLOR_AXIS); // left
        LCD_SetPointlColor(x+3, y+i, STREAM_COLOR_AXIS); // right
    }
}

static void draw_minus_sign(int x, int y) {
    // - pattern (4x2) - larger and thicker
    for (int i = 0; i < 4; i++) {
        LCD_SetPointlColor(x+i, y+2, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+i, y+3, STREAM_COLOR_AXIS);
    }
}

static void draw_letter_k(int x, int y) {
    // k pattern (4x6) - lowercase k
    // Vertical line
    for (int i = 0; i < 6; i++) {
        LCD_SetPointlColor(x+0, y+i, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+1, y+i, STREAM_COLOR_AXIS);
    }
    // Upper diagonal
    LCD_SetPointlColor(x+2, y+1, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+3, y+0, STREAM_COLOR_AXIS);
    // Middle connection
    LCD_SetPointlColor(x+2, y+2, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+2, y+3, STREAM_COLOR_AXIS);
    // Lower diagonal  
    LCD_SetPointlColor(x+3, y+4, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+3, y+5, STREAM_COLOR_AXIS);
}

static void draw_letter_v(int x, int y) {
    // V pattern (4x6) - uppercase V
    // Left diagonal
    LCD_SetPointlColor(x+0, y+0, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+0, y+1, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+1, y+2, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+1, y+3, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+2, y+4, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+2, y+5, STREAM_COLOR_AXIS);
    // Right diagonal
    LCD_SetPointlColor(x+3, y+0, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+3, y+1, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+2, y+2, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+2, y+3, STREAM_COLOR_AXIS);
}

static void draw_plus_sign(int x, int y) {
    // + pattern (4x4) - larger and thicker
    LCD_SetPointlColor(x+1, y+1, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+2, y+1, STREAM_COLOR_AXIS);
    for (int i = 0; i < 4; i++) {
        LCD_SetPointlColor(x+i, y+2, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x+i, y+3, STREAM_COLOR_AXIS);
    }
    LCD_SetPointlColor(x+1, y+4, STREAM_COLOR_AXIS);
    LCD_SetPointlColor(x+2, y+4, STREAM_COLOR_AXIS);
}

/**
 * Draw fixed axis labels and scale markers
 */
void fft_streaming_display_draw_axes(void) {
    // Draw thicker horizontal axis line (2 pixels thick)
    for (int x = STREAM_SPECTRUM_X; x < STREAM_SPECTRUM_X + STREAM_SPECTRUM_W; x++) {
        LCD_SetPointlColor(x, STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(x, STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H + 1, STREAM_COLOR_AXIS);
    }
    
    // Draw thicker vertical axis line (2 pixels thick)
    for (int y = STREAM_SPECTRUM_Y; y < STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H; y++) {
        LCD_SetPointlColor(STREAM_SPECTRUM_X, y, STREAM_COLOR_AXIS);
        LCD_SetPointlColor(STREAM_SPECTRUM_X - 1, y, STREAM_COLOR_AXIS);
    }
    
    // Horizontal axis (frequency) markers - main.cで定義された配列を使用
    printf("Axis frequency markers (%s scale):\n", USE_LOG_FREQ_SCALE ? "Logarithmic" : "Linear");
    
    for (int i = 0; i < FREQ_MARKERS_COUNT; i++) {
        // main.cで定義された周波数マーカーを使用
        uint32_t frequency = FREQ_MARKERS_HZ[i];
        
        // Use unified frequency-to-position function
        float normalized = fft_streaming_display_freq_to_position(frequency);
        int x = STREAM_SPECTRUM_X + (int)(normalized * STREAM_SPECTRUM_W);
        
        printf("  %dHz: norm=%.3f, x=%d (scale: %s)\n", 
                frequency, normalized, x, USE_LOG_FREQ_SCALE ? "Log" : "Linear");
        
        // Draw frequency marker tick (thicker and longer)
        for (int tick_y = 0; tick_y < 12; tick_y++) {
            LCD_SetPointlColor(x, STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H + 2 + tick_y, STREAM_COLOR_AXIS);
            LCD_SetPointlColor(x-1, STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H + 2 + tick_y, STREAM_COLOR_AXIS);
        }
        
        // Draw frequency labels - 5kHz刻みの動的表示
        int label_y = STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H + 18;
        
        // 周波数に応じて適切なラベルを描画 (1k, 5k, 10k, 15k, 20k, 25k, 30k, 35k, 40k, 45k, 50k)
        if (frequency == 1000) {
            // "1k"
            draw_digit_1(x - 6, label_y);
            draw_letter_k(x, label_y);
        } else if (frequency == 5000) {
            // "5k"
            draw_digit_5(x - 6, label_y);
            draw_letter_k(x, label_y);
        } else if (frequency == 10000) {
            // "10"
            draw_digit_1(x - 12, label_y);
            draw_digit_0(x - 6, label_y);
            draw_letter_k(x, label_y);
        } else if (frequency == 15000) {
            // "15"
            draw_digit_1(x - 12, label_y);
            draw_digit_5(x - 6, label_y);
            draw_letter_k(x, label_y);
        } else if (frequency == 20000) {
            // "20"
            draw_digit_2(x - 12, label_y);
            draw_digit_0(x - 6, label_y);
            draw_letter_k(x, label_y);
        } else if (frequency == 25000) {
            // "25"
            draw_digit_2(x - 12, label_y);
            draw_digit_5(x - 6, label_y);
            draw_letter_k(x, label_y);
        } else if (frequency == 30000) {
            // "30"
            draw_digit_3(x - 12, label_y);
            draw_digit_0(x - 6, label_y);
            draw_letter_k(x, label_y);
        } else if (frequency == 35000) {
            // "35"
            draw_digit_3(x - 12, label_y);
            draw_digit_5(x - 6, label_y);
            draw_letter_k(x, label_y);
        } else if (frequency == 40000) {
            // "40"
            draw_digit_4(x - 12, label_y);
            draw_digit_0(x - 6, label_y);
            draw_letter_k(x, label_y);
        } else if (frequency == 45000) {
            // "45"
            draw_digit_4(x - 12, label_y);
            draw_digit_5(x - 6, label_y);
            draw_letter_k(x, label_y);
        } else if (frequency == 50000) {
            // "50"
            draw_digit_5(x - 12, label_y);
            draw_digit_0(x - 6, label_y);
            draw_letter_k(x, label_y);
        }
    }
    
    // Vertical axis (amplitude) markers with linear dBm scale - 8レベル表示
    const int amp_markers = 8;
    int amplitude_dbm[8] = {20, 10, 0, -20, -40, -60, -80, -100};  // 上から下の順序（正しい）
    
    printf("Y-axis dBm markers (8 levels - linear): ");
    for (int i = 0; i < amp_markers; i++) {
        printf("%ddBm ", amplitude_dbm[i]);
    }
    printf("\n");
    
    // Debug: Show coordinate calculation
    printf("Coordinate calculation debug (Linear dBm Scale):\n");
    printf("STREAM_SPECTRUM_Y=%d, STREAM_SPECTRUM_H=%d\n", STREAM_SPECTRUM_Y, STREAM_SPECTRUM_H);
    printf("AMPLITUDE_RANGE: %d to %d dBm (Linear)\n", AMPLITUDE_RANGE_MIN_DB, AMPLITUDE_RANGE_MAX_DB);
    
    for (int i = 0; i < amp_markers; i++) {
        // Calculate position (linear scale for dBm) - main.cの範囲を使用
        float db_range_f = (float)(AMPLITUDE_RANGE_MAX_DB - AMPLITUDE_RANGE_MIN_DB);
        float normalized = (amplitude_dbm[i] - AMPLITUDE_RANGE_MIN_DB) / db_range_f;
        int y = STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H - (int)(normalized * STREAM_SPECTRUM_H);
        
        // Debug output for coordinate calculation
        printf("Level %d: %ddBm -> normalized=%.3f -> y=%d\n", i, amplitude_dbm[i], normalized, y);
        
        // Draw amplitude marker tick (thicker and longer)
        for (int tick_x = 0; tick_x < 12; tick_x++) {
            LCD_SetPointlColor(STREAM_SPECTRUM_X - 2 - tick_x, y, STREAM_COLOR_AXIS);
            LCD_SetPointlColor(STREAM_SPECTRUM_X - 2 - tick_x, y - 1, STREAM_COLOR_AXIS);
        }
        
        // Draw dBm level indicators (8レベル対応 - 線形dBm)
        int label_x = STREAM_SPECTRUM_X - 42;
        if (i == 0) {
            // "+20" indicator
            draw_plus_sign(label_x + 6, y - 3);
            draw_digit_2(label_x + 12, y - 3);
            draw_digit_0(label_x + 18, y - 3);
        } else if (i == 1) {
            // "+10" indicator
            draw_plus_sign(label_x + 6, y - 3);
            draw_digit_1(label_x + 12, y - 3);
            draw_digit_0(label_x + 18, y - 3);
        } else if (i == 2) {
            // "0" indicator
            draw_digit_0(label_x + 18, y - 3);
        } else if (i == 3) {
            // "-20" indicator
            draw_minus_sign(label_x + 6, y - 3);
            draw_digit_2(label_x + 12, y - 3);
            draw_digit_0(label_x + 18, y - 3);
        } else if (i == 4) {
            // "-40" indicator
            draw_minus_sign(label_x + 6, y - 3);
            draw_digit_4(label_x + 12, y - 3);
            draw_digit_0(label_x + 18, y - 3);
        } else if (i == 5) {
            // "-60" indicator
            draw_minus_sign(label_x + 6, y - 3);
            draw_digit_6(label_x + 12, y - 3);
            draw_digit_0(label_x + 18, y - 3);
        } else if (i == 6) {
            // "-80" indicator
            draw_minus_sign(label_x + 6, y - 3);
            draw_digit_8(label_x + 12, y - 3);
            draw_digit_0(label_x + 18, y - 3);
        } else if (i == 7) {
            // "-100" indicator
            draw_minus_sign(label_x, y - 3);
            draw_digit_1(label_x + 6, y - 3);
            draw_digit_0(label_x + 12, y - 3);
            draw_digit_0(label_x + 18, y - 3);
        }
    }
}

/**
 * Draw grid lines for better visualization
 */
void fft_streaming_display_draw_grid(void) {
    // Top border
    for (int x = STREAM_SPECTRUM_X - 2; x <= STREAM_SPECTRUM_X + STREAM_SPECTRUM_W + 1; x++) {
        LCD_SetPointlColor(x, STREAM_SPECTRUM_Y - 2, STREAM_COLOR_GRID);
    }
    // Bottom border
    for (int x = STREAM_SPECTRUM_X - 2; x <= STREAM_SPECTRUM_X + STREAM_SPECTRUM_W + 1; x++) {
        LCD_SetPointlColor(x, STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H + 1, STREAM_COLOR_GRID);
    }
    // Left border
    for (int y = STREAM_SPECTRUM_Y - 2; y <= STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H + 1; y++) {
        LCD_SetPointlColor(STREAM_SPECTRUM_X - 2, y, STREAM_COLOR_GRID);
    }
    // Right border
    for (int y = STREAM_SPECTRUM_Y - 2; y <= STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H + 1; y++) {
        LCD_SetPointlColor(STREAM_SPECTRUM_X + STREAM_SPECTRUM_W + 1, y, STREAM_COLOR_GRID);
    }
    
    // Draw horizontal grid lines (amplitude levels)
    for (int i = 1; i < 4; i++) {
        int y = STREAM_SPECTRUM_Y + (i * STREAM_SPECTRUM_H / 4);
        for (int x = STREAM_SPECTRUM_X; x < STREAM_SPECTRUM_X + STREAM_SPECTRUM_W; x += 10) {
            LCD_SetPointlColor(x, y, STREAM_COLOR_GRID);
        }
    }
    
    // Draw vertical grid lines (frequency markers)
    for (int i = 1; i < 8; i++) {
        int x = STREAM_SPECTRUM_X + (i * STREAM_SPECTRUM_W / 8);
        for (int y = STREAM_SPECTRUM_Y; y < STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H; y += 15) {
            LCD_SetPointlColor(x, y, STREAM_COLOR_GRID);
        }
    }
}

/**
 * スペクトラムストリーミング表示更新関数
 * 
 * 機能: FFT振幅データを用いてリアルタイムスペクトラム表示を更新する
 * 引数: 
 *   - magnitude_db: FFT振幅データ配列（dB値、FFT_SIZE/2要素）
 *   - sample_rate: 実際のサンプリング周波数（Hz）
 * 戻り値: なし
 * 
 * 処理内容:
 * - 30FPS高速表示に最適化された平滑化処理
 * - 周波数ビンから表示ピクセル位置への対数マッピング
 * - アンチフリッカー処理（指数移動平均）
 * - 設計要件範囲内での振幅制限（-100dB to +20dB）
 */
void fft_streaming_display_update_spectrum(const float* magnitude_db, float sample_rate) {
    if (!buffer_initialized) return;
    
    // 30FPS高速表示用アンチフリッカー平滑化バッファ（指数移動平均）
    // Smoothing buffer optimized for 30FPS high-speed display (exponential moving average)
    static float smooth_buffer[STREAM_BUFFER_COLS] = {0};
    static bool smooth_init = false;
    const float smooth_factor = 0.4f;  // 30FPS高速表示用に調整（0.3f→0.4f）- より応答性重視
    
    // Clear the entire spectrum buffer first
    for (int i = 0; i < STREAM_BUFFER_COLS; i++) {
        spectrum_buffer[i].x = STREAM_SPECTRUM_X + i;
        spectrum_buffer[i].y = STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H; // Bottom line (no signal)
    }
    
    // Map each FFT bin directly to its correct frequency position with smoothing
    for (int bin = 1; bin < STREAM_FFT_SIZE / 2; bin++) {  // Skip DC component (bin 0)
        // Convert FFT bin to frequency using actual sample rate
        float bin_freq = (float)bin * sample_rate / (float)STREAM_FFT_SIZE;
        
        // Skip frequencies outside our display range (100Hz - 50kHz)
        if (bin_freq < STREAM_FREQ_MIN_HZ || bin_freq > STREAM_FREQ_MAX_HZ) continue;
        
        // Convert frequency to display column using unified scaling function
        int col = fft_streaming_display_freq_to_column(bin_freq);
        
        // Get magnitude without noise gate (as per user requirement)
        float db_value = magnitude_db[bin];
        
        // Clamp dB value to design requirement range (-100 to +20 dBm)
        if (db_value < -100.0f) db_value = -100.0f;
        if (db_value > 20.0f) db_value = 20.0f;
        
        // Apply exponential moving average for smoothing
        if (!smooth_init) {
            smooth_buffer[col] = db_value;
        } else {
            smooth_buffer[col] = smooth_buffer[col] * (1.0f - smooth_factor) + db_value * smooth_factor;
        }
        
        // Peak hold functionality: 2-second peak hold implementation
        // ピークホールド機能: 2秒間のピーク保持実装（dBmベース）
        absolute_time_t current_time = get_absolute_time();
        if (smooth_buffer[col] > hold_buffer[col].peak_db || 
            absolute_time_diff_us(hold_buffer[col].hold_time, current_time) > PEAK_HOLD_DURATION_MS * 1000) {
            hold_buffer[col].peak_db = smooth_buffer[col];      // Update peak dBm
            hold_buffer[col].hold_time = current_time;          // Reset hold timer
        }
        
        // Convert smoothed dBm to pixel coordinates (linear scale: -100dBm to +20dBm)
        float db_range = (float)(AMPLITUDE_RANGE_MAX_DB - AMPLITUDE_RANGE_MIN_DB);
        float normalized_db = (smooth_buffer[col] - AMPLITUDE_RANGE_MIN_DB) / db_range;
        int height = (int)(normalized_db * STREAM_SPECTRUM_H);
        if (height < 0) height = 0;
        if (height >= STREAM_SPECTRUM_H) height = STREAM_SPECTRUM_H - 1;
        
        // Store in buffer with correct mapping
        spectrum_buffer[col].x = STREAM_SPECTRUM_X + col;
        spectrum_buffer[col].y = STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H - height;
        
        // Debug output for 1kHz frequency mapping verification (linear dBm scale)
        static int debug_counter = 0;
        if ((bin >= 7 && bin <= 9) && debug_counter % 180 == 0) {  // Bins around 1kHz, every 3 seconds at 60FPS
            float pos = fft_streaming_display_freq_to_position(bin_freq);
            printf("Freq mapping: bin %d (%.1fHz) -> col %d, pos %.3f, raw %.1fdBm, smooth %.1fdBm\n", 
                   bin, bin_freq, col, pos, db_value, smooth_buffer[col]);
        }
        debug_counter++;
    }
    
    smooth_init = true;
    
    // Render the spectrum display buffer
    fft_streaming_display_render_buffer();
}

/**
 * Render spectrum buffer to LCD
 * Anti-flashing optimized area updates
 */
void fft_streaming_display_render_buffer(void) {
    if (!buffer_initialized) return;
    
    // Clear and redraw only the spectrum area
    GUI_DrawRectangle(STREAM_SPECTRUM_X, STREAM_SPECTRUM_Y, 
                      STREAM_SPECTRUM_X + STREAM_SPECTRUM_W, 
                      STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H, 
                      STREAM_COLOR_BG, DRAW_FULL, DOT_PIXEL_1X1);
    
    // Draw vertical lines for each column in buffer
    for (int col = 0; col < STREAM_BUFFER_COLS; col++) {
        SpectrumPoint point = spectrum_buffer[col];
        if (point.x >= STREAM_SPECTRUM_X && point.x < STREAM_SPECTRUM_X + STREAM_SPECTRUM_W) {
            // Draw vertical line from bottom to point (current spectrum)
            for (int y = STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H - 1; y >= point.y; y--) {
                LCD_SetPointlColor(point.x, y, STREAM_COLOR_SPECTRUM);
            }
            
            // Draw peak hold line (設定可能ホールド時間) in cyan color
            // ピークホールド線の描画（main.cで設定可能）シアン色で表示
            // - 現在のスペクトラム（緑）の上にピーク値を水平線で表示
            // - 測定値の最大値を設定時間視覚的に保持し、ちらつきを防止
            float db_range = (float)(AMPLITUDE_RANGE_MAX_DB - AMPLITUDE_RANGE_MIN_DB);
            float hold_normalized_db = (hold_buffer[col].peak_db - AMPLITUDE_RANGE_MIN_DB) / db_range;
            int hold_height = (int)(hold_normalized_db * STREAM_SPECTRUM_H);
            if (hold_height < 0) hold_height = 0;
            if (hold_height >= STREAM_SPECTRUM_H) hold_height = STREAM_SPECTRUM_H - 1;
            int hold_y = STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H - hold_height;
            
            // Draw 2-pixel thick hold line for better visibility (enhanced for 2-second hold)
            // 視認性向上のため2ピクセル太の線で描画（2秒保持対応強化）
            if (hold_y >= STREAM_SPECTRUM_Y && hold_y < STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H) {
                LCD_SetPointlColor(point.x, hold_y, STREAM_HOLD_COLOR);
                if (hold_y > STREAM_SPECTRUM_Y) {
                    LCD_SetPointlColor(point.x, hold_y - 1, STREAM_HOLD_COLOR);
                }
            }
        }
    }
    
    // Redraw axis lines on top of spectrum
    fft_streaming_display_draw_axes();
}

/**
 * Get current spectrum display statistics
 */
void fft_streaming_display_get_stats(fft_streaming_display_stats_t* stats) {
    if (!stats || !buffer_initialized) return;
    
    stats->buffer_cols = STREAM_BUFFER_COLS;
    stats->update_width = STREAM_UPDATE_WIDTH;
    stats->spectrum_area_x = STREAM_SPECTRUM_X;
    stats->spectrum_area_y = STREAM_SPECTRUM_Y;
    stats->spectrum_area_w = STREAM_SPECTRUM_W;
    stats->spectrum_area_h = STREAM_SPECTRUM_H;
    stats->frequency_range_hz_min = 100;
    stats->frequency_range_hz_max = 50000;
    stats->amplitude_range_dbm_min = -100;
    stats->amplitude_range_dbm_max = 20;
}

/**
 * Test function: Display only axis labels for debugging
 */
void fft_streaming_display_test_axes_only(void) {
    printf("=== ENTERING TEST FUNCTION ===\n");
    printf("Testing axis display only...\n");
    
    // Clear entire screen first
    printf("Clearing LCD screen...\n");
    LCD_Clear(BLACK);
    printf("LCD cleared.\n");
    
    // Draw a large visible border around the entire screen for reference (320x240)
    GUI_DrawRectangle(5, 5, 315, 235, WHITE, DRAW_EMPTY, DOT_PIXEL_1X1);
    
    // Draw a border around the spectrum area for reference
    GUI_DrawRectangle(STREAM_SPECTRUM_X - 2, STREAM_SPECTRUM_Y - 2, 
                      STREAM_SPECTRUM_X + STREAM_SPECTRUM_W + 2, 
                      STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H + 2, 
                      RED, DRAW_EMPTY, DOT_PIXEL_1X1);
    
    // Draw the spectrum background area in dark gray to see it
    GUI_DrawRectangle(STREAM_SPECTRUM_X, STREAM_SPECTRUM_Y, 
                      STREAM_SPECTRUM_X + STREAM_SPECTRUM_W, 
                      STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H, 
                      0x2104, DRAW_FULL, DOT_PIXEL_1X1);  // Dark gray
    
    // Draw the axes FIRST - this is the main purpose of this test
    printf("Drawing fixed axis labels...\n");
    fft_streaming_display_draw_axes();
    printf("Axis labels drawn.\n");
    
    // Draw simple test lines (but don't cover the axes)
    printf("Drawing additional test elements...\n");
    
    // Draw horizontal line across the middle of spectrum area (not on axis)
    int test_y = STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H / 2;
    for (int x = STREAM_SPECTRUM_X + 20; x < STREAM_SPECTRUM_X + STREAM_SPECTRUM_W - 20; x++) {
        LCD_SetPointlColor(x, test_y, WHITE);
    }
    
    // Draw vertical line down the middle of spectrum area (not on axis)
    int test_x = STREAM_SPECTRUM_X + STREAM_SPECTRUM_W / 2;
    for (int y = STREAM_SPECTRUM_Y + 20; y < STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H - 20; y++) {
        LCD_SetPointlColor(test_x, y, WHITE);
    }
    
    // Draw large test squares in safe areas (not covering axes)
    printf("Drawing test squares...\n");
    
    // Test square at top-left of spectrum area
    for (int x = STREAM_SPECTRUM_X + 5; x < STREAM_SPECTRUM_X + 15; x++) {
        for (int y = STREAM_SPECTRUM_Y + 5; y < STREAM_SPECTRUM_Y + 15; y++) {
            LCD_SetPointlColor(x, y, WHITE);
        }
    }
    
    // Test square at bottom-right of spectrum area
    for (int x = STREAM_SPECTRUM_X + STREAM_SPECTRUM_W - 15; x < STREAM_SPECTRUM_X + STREAM_SPECTRUM_W - 5; x++) {
        for (int y = STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H - 15; y < STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H - 5; y++) {
            LCD_SetPointlColor(x, y, WHITE);
        }
    }
    
    printf("Axis and visual test complete. Check LCD display.\n");
    printf("You should see:\n");
    printf("- Fixed axis labels (100Hz-50kHz, -100dBm to +20dBm)\n");
    printf("- White border around entire screen\n");
    printf("- Red border around spectrum area at (%d,%d) to (%d,%d)\n", 
           STREAM_SPECTRUM_X, STREAM_SPECTRUM_Y,
           STREAM_SPECTRUM_X + STREAM_SPECTRUM_W, 
           STREAM_SPECTRUM_Y + STREAM_SPECTRUM_H);
    printf("- Dark gray spectrum background\n");
    printf("- White test lines in middle of spectrum area\n");
    printf("- White test squares in corners of spectrum area\n");
}
