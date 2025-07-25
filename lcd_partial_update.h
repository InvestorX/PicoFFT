/*****************************************************************************
* | File      	:   lcd_partial_update.h
* | Author      :   
* | Function    :   Partial LCD update for smooth animation
* | Info        :   
*   - Region-based updates
*   - Optimized for spectrum display
*   - Eliminates full-screen transfers
*----------------
******************************************************************************/

#ifndef __LCD_PARTIAL_UPDATE_H
#define __LCD_PARTIAL_UPDATE_H

#include "LCD_Driver.h"
#include "LCD_GUI.h"
#include <stdbool.h>

// Region structure for partial updates
typedef struct {
    POINT x1, y1, x2, y2;
    bool dirty;
} update_region_t;

// Maximum number of dirty regions to track
#define MAX_UPDATE_REGIONS 8

// Partial update manager
typedef struct {
    update_region_t regions[MAX_UPDATE_REGIONS];
    int region_count;
    bool enabled;
} partial_update_t;

// Function prototypes
bool partial_update_init(void);
void partial_update_cleanup(void);
void partial_update_mark_dirty(POINT x1, POINT y1, POINT x2, POINT y2);
void partial_update_clear_regions(void);
void partial_update_flush(void);
void partial_update_set_pixel(POINT x, POINT y, COLOR color);
void partial_update_draw_line(POINT x1, POINT y1, POINT x2, POINT y2, COLOR color);

// Global instance
extern partial_update_t g_partial_update;

#endif // __LCD_PARTIAL_UPDATE_H
