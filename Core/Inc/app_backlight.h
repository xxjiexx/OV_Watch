#ifndef APP_BACKLIGHT_H
#define APP_BACKLIGHT_H

#include <stdint.h>

void App_Backlight_Init(void);
void App_Backlight_SetBrightness(uint8_t percent);

#endif
