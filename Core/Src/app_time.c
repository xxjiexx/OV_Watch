#include "main.h"
#include "rtc.h"
#include "stm32f4xx_hal_rtc.h"
#include "stm32f4xx_hal_rtc_ex.h"
#include "app_time.h"

void App_Time_Init()
{
    /* RTC 的底层初始化和默认时间保护放在 MX_RTC_Init()，这里先保留应用层入口，后面可以扩展校时逻辑。 */
}

uint8_t App_Time_GetWeekDay(uint16_t year, uint8_t month, uint8_t day)
{
    if (month < 3)
    {
        month += 12;
        year--;
    }

    uint16_t century = year / 100;
    uint16_t yearInCentury = year % 100;
    uint8_t week = (day + 13 * (month + 1) / 5 + yearInCentury + yearInCentury / 4 + century / 4 + 5 * century) % 7;

    switch (week)
    {
        case 0:
            return RTC_WEEKDAY_SATURDAY;
        case 1:
            return RTC_WEEKDAY_SUNDAY;
        case 2:
            return RTC_WEEKDAY_MONDAY;
        case 3:
            return RTC_WEEKDAY_TUESDAY;
        case 4:
            return RTC_WEEKDAY_WEDNESDAY;
        case 5:
            return RTC_WEEKDAY_THURSDAY;
        case 6:
            return RTC_WEEKDAY_FRIDAY;
        default:
            return RTC_WEEKDAY_MONDAY;
    }
}

void App_Time_Set(uint16_t year, uint16_t mon, uint16_t day, uint16_t hour, uint16_t min, uint16_t sec)
{
    RTC_TimeTypeDef time = {0};
    RTC_DateTypeDef date = {0};

    /* 把 UI 传进来的时分秒写入 HAL 的 RTC 时间结构体。 */
    time.Hours = hour;
    time.Minutes = min;
    time.Seconds = sec;
    time.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    time.StoreOperation = RTC_STOREOPERATION_RESET;

    /* STM32 RTC 的年份只保存后两位，所以 2026 年写入 26。 */
    date.Year = year - 2000;
    date.Month = mon;
    date.Date = day;
    date.WeekDay = App_Time_GetWeekDay(year, mon, day);

    /* 用户点击保存时，应该无条件把新时间写入硬件 RTC。 */
    HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
    HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR0, RTC_INIT_MARK); // 写入标记，后续复位不要再覆盖默认时间。
}
