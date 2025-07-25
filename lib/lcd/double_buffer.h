/*****************************************************************************
* | File      	:   double_buffer.h
* | Author      :   
* | Function    :   Double buffering for smooth LCD updates
* | Info        :   
*   - Off-screen rendering
*   - Fast buffer swapping
*   - Reduced screen flicker
*----------------
******************************************************************************/

#ifndef __DOUBLE_BUFFER_H
#define __DOUBLE_BUFFER_H

#include "LCD_Driver.h"
#include "LCD_GUI.h"
#include <stdlib.h>
#include <string.h>

// Double buffer configuration
#define BUFFER_WIDTH  LCD_X_MAXPIXEL
#define BUFFER_HEIGHT LCD_Y_MAXPIXEL
#define BUFFER_SIZE   (BUFFER_WIDTH * BUFFER_HEIGHT)

// Double buffer structure
typedef struct {
    uint16_t* front_buffer;     // Currently displayed buffer
    uint16_t* back_buffer;      // Off-screen rendering buffer
    bool buffer_ready;          // Flag indicating back buffer is ready
    bool using_double_buffer;   // Enable/disable double buffering
} double_buffer_t;

// Function prototypes
bool double_buffer_init(void);
void double_buffer_cleanup(void);
void double_buffer_swap(void);
void double_buffer_clear(COLOR color);
void double_buffer_set_pixel(POINT x, POINT y, COLOR color);
void double_buffer_draw_line(POINT x1, POINT y1, POINT x2, POINT y2, COLOR color);
void double_buffer_draw_rectangle(POINT x1, POINT y1, POINT x2, POINT y2, COLOR color, bool filled);
void double_buffer_draw_text(POINT x, POINT y, const char* text, sFONT* font, COLOR bg_color, COLOR text_color);
void double_buffer_copy_to_lcd(void);
void double_buffer_wait_for_vblank(void);
void double_buffer_present_with_vsync(void);

// Global double buffer instance
extern double_buffer_t g_double_buffer;

#endif // __DOUBLE_BUFFER_H
