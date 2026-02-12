# Low-Power Firmware Example (STM32L053 + SHT40 + LCD)

This folder contains a minimal HAL-based example implementing:

- LCD always on (`MX_LCD_Init()` once, no LCD power-down in loop)
- Humidity measurement with SHT40 over I2C
- Active window from `07:00` to `17:59`
- Wake every 2 minutes inside active window
- Outside active window: sleep until next `07:00`
- STOP mode between wakeups for low power

## Files

- `main.c`: low-power scheduling + sensor sampling + RTC alarm wake
- `lcd_driver.h`, `lcd_driver.c`: LCD abstraction (you must add your segment mapping)

## Integration steps (CubeMX/CubeIDE)

1. Generate a project for `STM32L053` with enabled peripherals:
- `I2C1` (SHT40)
- `RTC` with LSE clock + Alarm A interrupt
- `LCD` peripheral for your segmented glass

2. Copy these files into your generated project:
- `Core/Src/main.c` (replace generated main loop logic)
- `Core/Src/lcd_driver.c`
- `Core/Inc/lcd_driver.h`

3. Keep generated init functions available in `main.c`:
- `MX_GPIO_Init()`
- `MX_I2C1_Init()`
- `MX_RTC_Init()`
- `MX_LCD_Init()`

4. Implement LCD segment mapping in `lcd_driver.c`:
- `LCD_DisplayHumidity()`
- `LCD_DisplayError()`

5. Verify RTC clock source is LSE (for best low-power accuracy).

## Notes

- The code keeps the last valid humidity value displayed even while sleeping.
- If a read fails, the example calls `LCD_DisplayError()`.
- If alarm setup fails, it falls back to a 2-second retry delay.
