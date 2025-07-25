cmake_minimum_required(VERSION 3.13)

# Create kiss_fft library
add_library(kiss_fft STATIC
    kiss_fft.c
)

target_include_directories(kiss_fft PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link with math library for floating point operations
target_link_libraries(kiss_fft m)
