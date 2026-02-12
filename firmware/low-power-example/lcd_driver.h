#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

#include <stdint.h>

void LCD_Init(void);
void LCD_DisplayHumidity(uint8_t humidity_percent);
void LCD_DisplayError(void);

#endif
