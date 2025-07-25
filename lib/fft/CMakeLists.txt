add_library(fft STATIC
    fft_analyzer_kiss.c
)

target_include_directories(fft PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ../kiss_fft
    ../../  # Root directory for config_settings.h
)

target_link_libraries(fft 
    kiss_fft
    pico_stdlib
    hardware_adc
    hardware_dma
    hardware_irq
    hardware_timer
    lcd
    pico_double
    m  # Math library for sin, cos, log10, sqrt functions
)
