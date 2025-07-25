# 查找当前目录下的所有源文件
# 并将名称保存到 DIR_examples_SRCS 变量
aux_source_directory(. DIR_EXAMPLES_SRCS)

include_directories(..)
include_directories(../lib/config)
include_directories(../lib/lcd)
include_directories(../lib/font)
include_directories(../lib/fatfs)
include_directories(../lib/sdcard)
include_directories(../lib/fft)

# 生成链接库 - Pico 2W最適化
add_library(examples ${DIR_EXAMPLES_SRCS})
target_link_libraries(examples PUBLIC config lcd font fatfs sdcard fft hardware_adc hardware_dma hardware_irq hardware_timer pico_stdlib pico_double)

# Pico 2W performance optimization for FFT analysis
target_compile_definitions(examples PRIVATE
    PICO_DOUBLE_SUPPORT_ROM_V1=0
    PICO_FLOAT_SUPPORT_ROM_V1=0
)
