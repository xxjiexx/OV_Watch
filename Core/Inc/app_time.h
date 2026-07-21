#ifndef APP_TIME_H
#define APP_TIME_H

#include <stdint.h>

#define RTC_INIT_MARK 0x2026U

void App_Time_Init();
uint8_t App_Time_GetWeekDay(uint16_t year, uint8_t month, uint8_t day);
void App_Time_Set(uint16_t year, uint16_t mon, uint16_t day, uint16_t hour, uint16_t min, uint16_t sec);

#endif
