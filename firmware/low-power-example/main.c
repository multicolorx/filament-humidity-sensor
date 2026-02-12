#include "main.h"
#include "lcd_driver.h"
#include <stdbool.h>
#include <stdint.h>

extern I2C_HandleTypeDef hi2c1;
extern RTC_HandleTypeDef hrtc;

void SystemClock_Config(void);

#define SHT40_I2C_ADDR               (0x44U << 1)
#define SHT40_MEAS_HIGH_PRECISION    0xFDU

#define ACTIVE_START_HOUR            7U
#define ACTIVE_END_HOUR_EXCLUSIVE    18U
#define ACTIVE_WAKE_MINUTES          2U

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} DateTime;

static volatile bool g_rtc_alarm_fired = false;

static uint8_t crc8_sht4x(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFFU;

    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8U; bit++) {
            if ((crc & 0x80U) != 0U) {
                crc = (uint8_t)((crc << 1) ^ 0x31U);
            } else {
                crc <<= 1;
            }
        }
    }

    return crc;
}

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t month_days[12] = {31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};
    uint8_t days = month_days[month - 1U];

    if (month == 2U) {
        bool leap = ((year % 4U) == 0U) && (((year % 100U) != 0U) || ((year % 400U) == 0U));
        if (leap) {
            days = 29U;
        }
    }

    return days;
}

static void increment_day(DateTime *dt)
{
    uint8_t dim = days_in_month(dt->year, dt->month);
    dt->day++;

    if (dt->day > dim) {
        dt->day = 1U;
        dt->month++;
        if (dt->month > 12U) {
            dt->month = 1U;
            dt->year++;
        }
    }
}

static void add_minutes(DateTime *dt, uint16_t minutes)
{
    uint16_t total = (uint16_t)dt->minute + minutes;

    dt->minute = (uint8_t)(total % 60U);
    dt->hour = (uint8_t)(dt->hour + (total / 60U));

    while (dt->hour >= 24U) {
        dt->hour = (uint8_t)(dt->hour - 24U);
        increment_day(dt);
    }
}

static bool read_now(DateTime *dt)
{
    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};

    if (HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN) != HAL_OK) {
        return false;
    }

    if (HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN) != HAL_OK) {
        return false;
    }

    dt->year = (uint16_t)(2000U + d.Year);
    dt->month = d.Month;
    dt->day = d.Date;
    dt->hour = t.Hours;
    dt->minute = t.Minutes;
    dt->second = t.Seconds;

    return true;
}

static bool in_active_window(const DateTime *dt)
{
    return (dt->hour >= ACTIVE_START_HOUR) && (dt->hour < ACTIVE_END_HOUR_EXCLUSIVE);
}

static void compute_next_wake(const DateTime *now, DateTime *next)
{
    *next = *now;
    next->second = 0U;

    if (in_active_window(now)) {
        add_minutes(next, ACTIVE_WAKE_MINUTES);
        return;
    }

    if (now->hour < ACTIVE_START_HOUR) {
        next->hour = ACTIVE_START_HOUR;
        next->minute = 0U;
        return;
    }

    increment_day(next);
    next->hour = ACTIVE_START_HOUR;
    next->minute = 0U;
}

static bool set_alarm_for_datetime(const DateTime *dt)
{
    RTC_AlarmTypeDef alarm = {0};

    alarm.AlarmTime.Hours = dt->hour;
    alarm.AlarmTime.Minutes = dt->minute;
    alarm.AlarmTime.Seconds = dt->second;
    alarm.AlarmTime.SubSeconds = 0U;
    alarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    alarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;

    alarm.AlarmMask = RTC_ALARMMASK_NONE;
    alarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
    alarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
    alarm.AlarmDateWeekDay = dt->day;
    alarm.Alarm = RTC_ALARM_A;

    (void)HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
    __HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);

    return HAL_RTC_SetAlarm_IT(&hrtc, &alarm, RTC_FORMAT_BIN) == HAL_OK;
}

static bool read_humidity_percent(uint8_t *humidity_percent)
{
    uint8_t cmd = SHT40_MEAS_HIGH_PRECISION;
    uint8_t raw[6] = {0};

    if (HAL_I2C_Master_Transmit(&hi2c1, SHT40_I2C_ADDR, &cmd, 1U, HAL_MAX_DELAY) != HAL_OK) {
        return false;
    }

    HAL_Delay(10U);

    if (HAL_I2C_Master_Receive(&hi2c1, SHT40_I2C_ADDR, raw, sizeof(raw), HAL_MAX_DELAY) != HAL_OK) {
        return false;
    }

    if (crc8_sht4x(&raw[0], 2U) != raw[2] || crc8_sht4x(&raw[3], 2U) != raw[5]) {
        return false;
    }

    uint16_t raw_rh = (uint16_t)((raw[3] << 8) | raw[4]);
    int32_t rh_x100 = ((12500L * raw_rh) / 65535L) - 600L;

    if (rh_x100 < 0L) {
        *humidity_percent = 0U;
    } else if (rh_x100 > 10000L) {
        *humidity_percent = 100U;
    } else {
        *humidity_percent = (uint8_t)((rh_x100 + 50L) / 100L);
    }

    return true;
}

static void enter_stop_mode(void)
{
    HAL_SuspendTick();
    HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
    HAL_ResumeTick();

    SystemClock_Config();
}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc_instance)
{
    if (hrtc_instance->Instance == RTC) {
        g_rtc_alarm_fired = true;
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_RTC_Init();
    MX_LCD_Init();

    HAL_PWREx_EnableUltraLowPower();
    HAL_PWREx_EnableFastWakeUp();

    LCD_Init();

    uint8_t last_humidity = 0U;
    if (read_humidity_percent(&last_humidity)) {
        LCD_DisplayHumidity(last_humidity);
    }

    for (;;) {
        DateTime now = {0};
        DateTime next = {0};

        if (read_now(&now)) {
            if (in_active_window(&now)) {
                uint8_t humidity = 0U;
                if (read_humidity_percent(&humidity)) {
                    last_humidity = humidity;
                    LCD_DisplayHumidity(last_humidity);
                } else {
                    LCD_DisplayError();
                }
            } else {
                LCD_DisplayHumidity(last_humidity);
            }

            compute_next_wake(&now, &next);
            if (set_alarm_for_datetime(&next)) {
                g_rtc_alarm_fired = false;
                while (!g_rtc_alarm_fired) {
                    enter_stop_mode();
                }
                continue;
            }
        }

        HAL_Delay(2000U);
    }
}
