# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)
include(pico_sdk_import.cmake)
project(PicoFFT) 
pico_sdk_init()

add_subdirectory(lib/config)
add_subdirectory(lib/lcd)
add_subdirectory(lib/font)
add_subdirectory(lib/fatfs)
add_subdirectory(lib/sdcard)
add_subdirectory(lib/kiss_fft)
add_subdirectory(lib/fft)

include_directories(lib)
include_directories(.)
include_directories(./lib/config)
include_directories(./lib/lcd)
include_directories(./lib/font)
include_directories(./lib/fatfs)
include_directories(./lib/sdcard)
include_directories(./lib/fft)
include_directories(./lib/kiss_fft)

add_executable(PicoFFT 
main.c
lib/lcd_test.c
fft_streaming_display.c
adc_sampling.c
fft_realtime_unified.c
)

# Pico 2W specific optimizations for high-performance FFT
target_compile_definitions(PicoFFT PRIVATE
    PICO_DOUBLE_SUPPORT_ROM_V1=0  # Use native ARM double precision for better performance
    PICO_FLOAT_SUPPORT_ROM_V1=0   # Use native ARM float for better performance
    PICO_STDIO_USB_TASK_INTERVAL_US=1000  # Faster USB communication
)

# enable usb output, disable uart output
pico_enable_stdio_usb(PicoFFT 1)
pico_enable_stdio_uart(PicoFFT 0)

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(PicoFFT)

target_link_libraries(PicoFFT lcd font config fft kiss_fft pico_stdlib hardware_spi hardware_adc hardware_dma hardware_irq hardware_timer pico_double fatfs)
