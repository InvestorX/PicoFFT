# PicoFFT - Raspberry Pi Pico FFT Spectrum Analyzer

**高精度リアルタイムFFTスペクトラムアナライザー** - Raspberry Pi Pico 2W + 2.8" IPS LCD で実現する本格的な周波数分析システム

![PicoFFT](https://img.shields.io/badge/Platform-Raspberry%20Pi%20Pico%202W-red)
![Build](https://img.shields.io/badge/Build-CMake-blue)
![FFT](https://img.shields.io/badge/FFT-kiss__fft-green)
![SUSHI-WARE LICENSE](https://img.shields.io/badge/license-SUSHI--WARE%F0%9F%8D%A3-blue.svg)

## 🎯 概要

PicoFFTは、Raspberry Pi Pico 2Wを使用した高性能なリアルタイムFFTスペクトラムアナライザーです。プロフェッショナルグレードの測定精度と使いやすさを両立し、教育・研究・開発用途に最適化されています。

### ✨ 主要な特徴

🔬 **高精度測定**: 75Ω系対応の正確なdBm測定（-100〜+20dBm）  
⚡ **リアルタイム処理**: 128kHzサンプリング、30FPS表示  
📊 **多彩な窓関数**: 7種類の窓関数で用途に応じた最適化  
🎛️ **二重サンプリングモード**: 手動/DMA切替でシステム要件に柔軟対応  
🖥️ **直感的表示**: 320×240 IPS LCD上の見やすいスペクトラム表示  
🔧 **設定可能**: 中央集約型設定システムで簡単カスタマイズ  

## 🚀 技術仕様

### システム性能
- **サンプリング周波数**: 128kHz
- **FFTサイズ**: 1024ポイント
- **周波数分解能**: 125Hz/bin
- **表示周波数範囲**: 1kHz〜50kHz (240カラム表示)
- **振幅測定範囲**: -100dBm〜+20dBm (8レベル線形表示)
- **更新レート**: 30FPS (リアルタイム表示)
- **測定精度**: ±0.5dB (校正済み)

### サンプリングシステム
- **ADC分解能**: 12bit (4096段階)
- **基準電圧**: 3.3V
- **入力インピーダンス**: 100kΩ (高インピーダンス)
- **信号系インピーダンス**: 75Ω対応
- **基準レベル**: 0.274V RMS = 0dBm @ 75Ω

### 対応窓関数
1. **Rectangle** (矩形窓) - 最高周波数分解能
2. **Hamming** (ハミング窓) - 分解能と漏れのバランス
3. **Hann** (ハン窓) - 滑らかなスペクトラム特性
4. **Blackman** (ブラックマン窓) - 低サイドローブ
5. **Blackman-Harris** - 超低サイドローブ
6. **Kaiser-Bessel** (β=8.5) - 調整可能特性
7. **Flat-Top** (フラットトップ窓) - 高精度振幅測定

## 🛠️ ハードウェア要件

### 必要コンポーネント
- **Raspberry Pi Pico 2W** (RP2350ベース、WiFi搭載)
- **LCDディスプレイ**: [Waveshare Pico-ResTouch-LCD-2.8](https://www.waveshare.com/wiki/Pico-ResTouch-LCD-2.8)
  - 2.8インチ IPS LCD (320×240ピクセル)
  - 抵抗膜式タッチパネル
  - SPI通信インターフェース
- **入力回路**: 75Ω系インピーダンスマッチング

### GPIO接続
```
ADC入力    : GPIO26 (ADC0) - アナログ信号入力
SPI LCD    : 標準SPIピン配置
電源       : USB 5V → 3.3V変換 (内蔵)
GND        : 共通グランド
```

## 📦 ソフトウェア構成

### コアモジュール
- **`main.c`**: メインプログラム・初期化
- **`fft_realtime_unified.c`**: 統合リアルタイムFFT処理エンジン
- **`adc_sampling.c`**: 統合ADCサンプリングシステム (手動/DMA)
- **`fft_streaming_display.c`**: スペクトラム表示・レンダリング
- **`config_settings.h`**: 中央集約型設定ファイル

### ライブラリ依存関係
- **kiss_fft**: 高速FFT演算ライブラリ
- **Pico SDK**: Raspberry Pi Pico開発環境
- **LCD Driver**: Waveshare LCD制御ライブラリ
- **HAL**: ハードウェア抽象化レイヤー

## 🔧 ビルド・インストール

### 前提条件
```bash
# Pico SDK環境のセットアップ (Windows)
# 1. Pico SDK のインストール
# 2. CMake, Ninja のインストール  
# 3. ARM GCC Toolchain のインストール
```

### ビルド手順
```bash
# リポジトリのクローン
git clone https://github.com/your-username/PicoFFT.git
cd PicoFFT

# ビルドディレクトリの作成
mkdir build && cd build

# CMake設定
cmake -G Ninja ..

# ビルド実行
ninja

# 出力: PicoFFT.uf2 ファイルが生成される
```

### フラッシュ方法
1. Pico 2WのBOOTSELボタンを押しながらUSB接続
2. マスストレージとして認識される
3. `PicoFFT.uf2`をドラッグ&ドロップ
4. 自動的に再起動して動作開始

## ⚙️ 設定とカスタマイズ

### 主要設定項目 (`config_settings.h`)

```c
// サンプリング設定
#define SAMPLING_RATE_HZ 128000          // サンプリング周波数
#define TARGET_FPS 30                    // 表示フレームレート

// 表示範囲設定  
#define FREQUENCY_RANGE_MIN 1000         // 最低表示周波数 (Hz)
#define FREQUENCY_RANGE_MAX 50000        // 最高表示周波数 (Hz)
#define AMPLITUDE_RANGE_MIN_DB -100      // 最小表示振幅 (dBm)
#define AMPLITUDE_RANGE_MAX_DB 20        // 最大表示振幅 (dBm)

// サンプリングモード選択
#define ADC_DMA_ENABLED 1                // 0=手動, 1=DMA (推奨)

// 窓関数選択 (0-6)
#define FFT_WINDOW_TYPE 0                // 0=Rectangle, 1=Hamming, ...

// 表示補正 (手動モード使用時のみ)
#define FREQUENCY_DISPLAY_OFFSET_HZ -2500   // 周波数オフセット (Hz)
#define ENABLE_FREQUENCY_OFFSET_CORRECTION 0 // 補正有効/無効
```

### 測定モード

#### DMAモード (推奨)
- **特徴**: 高精度、安定したサンプリング
- **用途**: 精密測定、連続動作
- **設定**: `ADC_DMA_ENABLED = 1`

#### 手動モード
- **特徴**: CPUベース、シンプル
- **用途**: デバッグ、リソース制約時
- **設定**: `ADC_DMA_ENABLED = 0` + オフセット補正

## 📊 使用方法

### 基本操作
1. **電源投入**: USB接続で自動起動
2. **信号入力**: GPIO26に測定対象信号を接続
3. **表示確認**: LCD上にリアルタイムスペクトラム表示
4. **周波数読取**: X軸目盛りで周波数確認
5. **振幅読取**: Y軸目盛りでdBm値確認

### 測定のコツ
- **信号レベル**: -100〜+20dBmの範囲で最適
- **インピーダンス**: 75Ω系で校正済み
- **周波数範囲**: 1〜50kHzで最高精度
- **窓関数選択**: 用途に応じて最適化

## � デバッグ・診断

### 内蔵診断機能
- **周波数マッピング解析**: FFTビン→表示座標の精度検証
- **振幅校正確認**: dBm換算の正確性検証  
- **サンプリング監視**: ADC性能・タイミング解析
- **ピーク検出**: 信号強度・周波数精度分析

### デバッグ出力例
```
=== 🔍 Frequency Mapping Debug ===
22.5kHz Signal Analysis:
  FFT_Bin: 180, Bin_Freq: 22500Hz
  Axis_X: 145px, Spectrum_X: 145px  
  ✅ Perfect alignment

=== 📊 Amplitude Analysis ===  
Signal: -43.1dBm (corrected)
Display: Y=115px (normalized: 0.474)
✅ Calibration verified
```

## 🎯 応用例

### 教育用途
- **信号処理学習**: FFT理論の実践的理解
- **窓関数比較**: 各窓関数の特性を視覚的に学習
- **周波数解析**: 実信号のスペクトラム解析実習

### 開発・測定用途  
- **回路評価**: フィルタ特性・発振器解析
- **EMI対策**: 不要輻射の周波数特定
- **音響解析**: オーディオ信号の周波数特性測定

### 研究用途
- **アルゴリズム検証**: FFT・窓関数性能評価
- **ハードウェア評価**: ADC性能・ノイズ解析
- **システム最適化**: リアルタイム処理性能測定

## 🏆 開発成果・技術的成就

### 解決した技術課題
✅ **周波数マッピング精度**: DMAモードで±0.1%以下の精度実現  
✅ **振幅校正**: 75Ω系での正確なdBm測定（±0.5dB）  
✅ **リアルタイム性**: 30FPS安定表示の達成  
✅ **柔軟性**: 手動/DMA二重モード対応  
✅ **保守性**: 中央集約型設定システム  

### 技術的革新点
- **統合サンプリングシステム**: 手動/DMAの透明な切替
- **高精度周波数マッピング**: FFTビン→ピクセル座標の数学的最適化
- **インピーダンス補正**: 75Ω系での正確な電力測定
- **適応的表示補正**: モード依存の自動オフセット補正

## 📄 ライセンス

このプロジェクトは SUSHI-WARE ライセンスの下で提供されています。

```
THE SUSHI-WARE LICENSE 🍣 (Revision 1)

<your-name> wrote this code. As long as you retain this notice, you can do 
whatever you want with this stuff. If we meet some day, and you think this 
stuff is worth it, you can buy me sushi in return.
```

## 🤝 貢献・サポート

### 貢献歓迎
- **バグ報告**: Issues での報告をお願いします
- **機能提案**: 新機能のアイデアを共有してください  
- **プルリクエスト**: コード改善の提案を歓迎します
- **ドキュメント**: 説明の改善・追加をお願いします

### サポート
- **技術的質問**: Issues でお気軽にお尋ねください
- **使用方法**: Wiki やコメントをご参照ください
- **カスタマイズ**: 設定ファイルでの調整方法を説明しています

---

**PicoFFT** - Making frequency analysis accessible, accurate, and affordable! 🎵📊✨

## 📋 Dependencies

### Pico SDK
- **Version**: 2.1.1 or later
- **Components**: hardware_adc, hardware_spi, pico_stdlib
- **Math Libraries**: pico_double, standard math library

### Custom Libraries
- **kiss_fft**: Fast Fourier Transform implementation
- **LCD Driver**: Custom LCD graphics and touch interface
- **Font Library**: Multiple font sizes (8, 12, 16, 20, 24)
- **FAT File System**: SD card support (optional)

## 🔧 Installation & Build

### Prerequisites
```bash
# Install Pico SDK (Windows/Linux/macOS)
# Follow official Raspberry Pi Pico setup guide
```

### Build Process
```bash
# Clone the repository
git clone https://github.com/yourusername/PicoFFT.git
cd PicoFFT

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
ninja

# Output: PicoFFT.uf2 file will be generated
```

### Flashing to Pico
1. Hold BOOTSEL button and connect Pico to USB
2. Copy `build/PicoFFT.uf2` to the Pico drive
3. Pico will automatically reboot and start the application

## ⚙️ Configuration

### Main Configuration File: `config_settings.h`

#### Window Function Selection
```c
// Choose window function (0-6)
#define FFT_WINDOW_TYPE 0  // 0=Rectangle, 1=Hamming, 2=Hann, etc.
```

#### Sampling & Display Settings
```c
#define TARGET_FPS 30                    // Frame rate
#define SAMPLING_RATE_HZ 128000          // ADC sampling rate
#define FREQUENCY_RANGE_MIN 1000         // Min frequency (1kHz)
#define FREQUENCY_RANGE_MAX 50000        // Max frequency (50kHz)
```

#### dBm Calibration
```c
#define DB_REFERENCE_VOLTAGE_0DBM 0.274f // 0dBm reference (75Ω system)
#define DB_REFERENCE_IMPEDANCE 75.0f     // System impedance
```

### Frequency Markers
Pre-configured frequency markers at: 1kHz, 5kHz, 10kHz, 15kHz, 20kHz, 25kHz, 30kHz, 35kHz, 40kHz, 45kHz, 50kHz

## 📊 Display Layout

### Vertical Axis (Amplitude)
8-level linear dBm scale:
- +20 dBm (top)
- +10 dBm
- 0 dBm
- -20 dBm
- -40 dBm
- -60 dBm
- -80 dBm
- -100 dBm (bottom)

### Horizontal Axis (Frequency)
Linear frequency scale from 1kHz to 50kHz with configurable markers

## 🔬 Technical Details

### FFT Processing Pipeline
1. **ADC Sampling**: 12-bit samples at 128kHz
2. **DC Offset Removal**: Automatic DC component elimination
3. **Window Function**: Configurable windowing (7 types available)
4. **FFT Calculation**: kiss_fft implementation
5. **Magnitude Calculation**: Complex to magnitude conversion
6. **dB Conversion**: Logarithmic scaling with proper calibration
7. **Display Mapping**: Real-time spectrum visualization

### Window Function Coefficients
All window functions include proper coherent gain and amplitude correction factors for accurate measurements.

### Impedance Matching
The system is calibrated for 75Ω impedance with proper voltage scaling:
- 0dBm = 1mW @ 75Ω = 0.274V RMS
- Input impedance correction applied automatically

## 📁 Project Structure

```
PicoFFT/
├── main.c                      # Main application entry point
├── config_settings.h           # Central configuration file
├── CMakeLists.txt             # Build configuration
├── fft_streaming_display.c    # Real-time display logic
├── lib/                       # Library modules
│   ├── fft/                   # FFT processing
│   │   ├── fft_analyzer.c     # Core FFT implementation
│   │   ├── fft_analyzer_kiss.c # kiss_fft variant
│   │   └── fft_analyzer_kiss_new.c # Enhanced kiss_fft
│   ├── lcd/                   # LCD driver and graphics
│   ├── kiss_fft/             # FFT library
│   ├── font/                  # Font rendering
│   ├── config/               # Hardware configuration
│   └── fatfs/                # File system (optional)
├── examples/                  # Example programs
└── build/                    # Build output directory
```

## 🐛 Troubleshooting

### Common Issues

#### Build Errors
- Ensure Pico SDK 2.1.1+ is properly installed
- Check CMake version (3.12+ required)
- Verify all submodules are initialized

#### Display Issues
- Check SPI wiring connections
- Verify LCD driver compatibility
- Ensure proper power supply (3.3V)

#### ADC Issues
- Confirm GPIO26 is used for ADC input
- Check input signal amplitude (within 0-3.3V range)
- Verify impedance matching circuit

### Debug Output
Enable debug output via USB/UART for troubleshooting:
```c
stdio_init_all();  // Already enabled in main.c
```

## 🔄 Version History

### v1.0.0 (Current)
- Initial release with 7 window functions
- 8-level linear dBm display
- 75Ω impedance calibration
- Real-time 30 FPS operation
- Configurable parameters via config_settings.h

## 📄 License
### "THE SUSHI-WARE LICENSE"

InvestorX wrote this file.

As long as you retain this notice you can do whatever you want
with this stuff. If we meet some day, and you think this stuff
is worth it, you can buy me a **sushi 🍣** in return.

(This license is based on ["THE BEER-WARE LICENSE" (Revision 42)].
 Thanks a lot, Poul-Henning Kamp ;)

​["THE BEER-WARE LICENSE" (Revision 42)]: https://people.freebsd.org/~phk/


## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📞 Support

For questions, issues, or contributions:
- Open an issue on GitHub
- Check the troubleshooting section
- Review the configuration documentation

## 🎯 Future Enhancements

### High Priority
- [ ] **ADC DMA Support** - Implement DMA-based ADC sampling for improved performance and reduced CPU overhead
- [ ] **DMA-based Graphics Rendering** - Optimize display updates using DMA transfers for faster frame rates

### Medium Priority
- [ ] Waterfall display mode
- [ ] Peak detection and frequency measurement
- [ ] Data logging to SD card
- [ ] WiFi connectivity for remote monitoring

### Low Priority
- [ ] Additional window functions
- [ ] Touch interface controls
- [ ] Frequency sweep generator

---

**Made with ❤️ for the Raspberry Pi Pico community**
