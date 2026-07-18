#include "main.h"
#include "rtc.h"
#include "stm32f4xx_hal_rtc.h"
#include "app_ui.h"

typedef struct{
    Hours;
    Minutes;
    Seconds;
}RTC_TimeTypeDef;

typedef struct{
    Years;
    Month;
    Days;
}RTC_DateTypeDef;

void App_Time_Init()
{
    HAL_RTC_Init();
}

void App_Time_Set(uint16_t year, uint16_t mon, uint16_t day, uint16_t hour, uint16_t min, uint16_t sec)
{
    hour = lv_roller_get_selected(hour_rollor);
    min = lv_roller_get_selected(min_rollor);
    sec = lv_roller_get_selected(sec_rollor);
    year = 2020U + lv_roller_get_selected(year_rollor);
    mon = 1U + lv_roller_get_selected(mon_rollor);
    day = 1U + lv_roller_get_selected(day_rollor);

    HAL_RTC_SetTime(&hrtc, &time, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &date, RTC_FORMAT_BIN);
}
