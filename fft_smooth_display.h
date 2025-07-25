/*****************************************************************************
* | File      	:   fft_smooth_display.h
* | Author      :   
* | Function    :   Ultra-smooth FFT display header
* | Info        :   
*   - Optimized for minimal LCD transfers
*   - Smooth animation capability
*   - Partial update based
*----------------
******************************************************************************/

#ifndef __FFT_SMOOTH_DISPLAY_H
#define __FFT_SMOOTH_DISPLAY_H

#include <stdbool.h>
#include "fft_analyzer.h"

// Function prototypes
bool fft_smooth_display_init(void);
void fft_smooth_display_cleanup(void);
void fft_smooth_display_draw_background(void);
void fft_smooth_display_update_spectrum(const float* magnitude_db);
void fft_smooth_display_show_info(float fps);
void fft_smooth_display_present(void);

#endif // __FFT_SMOOTH_DISPLAY_H
