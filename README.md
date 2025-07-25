# PicoFFT - Raspberry Pi Pico FFT Spectrum Analyzer

Real-time FFT spectrum analyzer implementation for Raspberry Pi Pico with LCD display, featuring configurable window functions and precision dBm measurements.

![PicoFFT](https://img.shields.io/badge/Platform-Raspberry%20Pi%20Pico-red)
![Build](https://img.shields.io/badge/Build-CMake-blue)

## 🚀 Features

### Core Functionality
- **Real-time FFT Analysis**: 128kHz sampling rate with configurable FFT processing
- **Multi-Window Functions**: 7 different window functions including Rectangle, Hamming, Hann, Blackman, etc.
- **Precision dBm Display**: Linear 8-level dBm scale (+20 to -100 dBm) with proper 75Ω impedance calibration
- **LCD Graphics**: Real-time spectrum display with frequency markers and amplitude scaling
- **Configurable Parameters**: Centralized configuration system for easy customization

### Supported Window Functions
1. **Rectangle** (Rectangular) - Maximum frequency resolution
2. **Hamming** - Good compromise between resolution and leakage
3. **Hann** (Hanning) - Smooth spectral characteristics
4. **Blackman** - Low side lobe levels
5. **Blackman-Harris** - Ultra-low side lobe levels
6. **Kaiser-Bessel** (β=8.5) - Adjustable characteristics
7. **Flat-Top** - Accurate amplitude measurements

### Technical Specifications
- **Sampling Rate**: 128kHz
- **Frequency Range**: 1kHz - 50kHz
- **Amplitude Range**: -100dBm to +20dBm (8 linear levels)
- **Target Frame Rate**: 30 FPS
- **ADC Resolution**: 12-bit
- **Reference Impedance**: 75Ω system
- **Reference Voltage**: 0.274V RMS (0dBm @ 75Ω)

## 🛠️ Hardware Requirements

### Main Components
- **Raspberry Pi Pico 2W** (or compatible Pico board)
- **LCD Display**: [Waveshare Pico-ResTouch-LCD-2.8](https://www.waveshare.com/wiki/Pico-ResTouch-LCD-2.8)
  - 2.8" IPS LCD with 320×240 resolution
  - Resistive touch panel
  - SPI interface
- **ADC Input Circuit** (75Ω impedance matching)

### Pin Configuration
- **ADC Input**: GPIO26 (ADC0)
- **SPI Display**: Standard SPI pins for LCD communication
- **Power**: 3.3V system with USB power

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
