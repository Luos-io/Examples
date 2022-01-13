/******************************************************************************
 * @file lcd
 * @brief driver example a simple lcd
 * @author mariebidouille
 * @version 0.0.0
 ******************************************************************************/
#ifndef LCD_H
#define LCD_H

#include "luos.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
typedef struct __attribute__((__packed__))
{
    union
    {
        struct __attribute__((__packed__))
        {
            uint8_t mode_display : 1; 
            uint8_t mode_cursor : 1;
            uint8_t mode_blink : 1;
            uint8_t mode_autoscroll : 1;
            uint8_t mode_scroll_display : 1;
            uint8_t mode_right_to_left : 1;
        };
        uint8_t unmap[2];
    };
} lcd_mode_t;

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Functions
 ******************************************************************************/
void Lcd_Init(void);
void Lcd_Loop(void);

#endif /* LCD_H */
