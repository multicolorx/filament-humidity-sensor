#include "lcd_driver.h"
#include "main.h"

extern LCD_HandleTypeDef hlcd;

void LCD_Init(void)
{
    (void)hlcd;
    // Keep LCD peripheral enabled continuously so the display remains visible in STOP mode.
}

void LCD_DisplayHumidity(uint8_t humidity_percent)
{
    (void)humidity_percent;
    // TODO: Implement segment mapping for your glass and write digits via HAL_LCD_Write().
}

void LCD_DisplayError(void)
{
    // TODO: Show an error pattern (for example "Er") when SHT40 read fails.
}
