#ifndef APP_SETTINGS_H
#define APP_SETTINGS_H

#include <stdint.h>

#define APP_SETTINGS_MAGIC 0x57544348U
#define APP_SETTINGS_FLASH_ADDR 0x08060000U


typedef struct{
  uint32_t magic;
  uint8_t watch_face;
  uint8_t brightness;
  uint32_t screen_timeout_ms;
  uint32_t checksum;
}AppSettings;

void App_Settings_Init(void);
const AppSettings *App_Settings_Get(void);
void App_Settings_Save(const AppSettings *settings);
void App_Settings_SetBrightness(uint8_t brightness);
void App_Settings_SetWatchFace(uint8_t watch_face);
void App_Settings_SetScreenTimeout(uint32_t timeout_ms);

#endif