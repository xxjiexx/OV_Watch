#include "app_settings.h"
#include "main.h"
#include <stdint.h>

static const AppSettings default_settings = {
    .magic = APP_SETTINGS_MAGIC,
    .brightness = 80,
    .checksum = 0,
    .screen_timeout_ms =10000,
    .watch_face = 0,
};
static AppSettings current_settings;

static uint32_t App_Settings_Checksum(const AppSettings* settings)
{
    uint32_t sum = 0;
    const uint8_t *data = (const uint8_t *)settings;

    for(uint32_t i = 0; i < sizeof(AppSettings) - sizeof(settings -> checksum); i++)
    {
        sum +=data[i];
    }

    return sum;
}

void App_Settings_Init(void)
{
    const AppSettings *flash_settings = (const AppSettings *)APP_SETTINGS_FLASH_ADDR;

    if(flash_settings->magic == APP_SETTINGS_MAGIC &&
       App_Settings_Checksum(flash_settings) == flash_settings->checksum)
    {
        current_settings = *flash_settings; // Flash 里的设置合法，复制到 RAM 运行时使用。
    }
    else
    {
        current_settings = default_settings; // Flash 没保存过或数据坏了，就用默认设置。
    }
}
const AppSettings *App_Settings_Get(void)
{
    return &current_settings;
}
void App_Settings_Save(const AppSettings *settings)
{
    AppSettings save_settings = *settings;

    uint32_t sector_error = 0;
    FLASH_EraseInitTypeDef erase = {0};
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.NbSectors = 1;
    erase.Sector = FLASH_SECTOR_7;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    uint32_t address = APP_SETTINGS_FLASH_ADDR;
    const uint32_t *data = (const uint32_t *)&save_settings;

    HAL_StatusTypeDef Erase_State = 0;
    
    save_settings.magic = APP_SETTINGS_MAGIC;
    save_settings.checksum = App_Settings_Checksum(&save_settings);

    HAL_FLASH_Unlock();

    Erase_State = HAL_FLASHEx_Erase(&erase, &sector_error);
    
    if(Erase_State == HAL_OK)
    {
        for(uint32_t i = 0; i < sizeof(save_settings) / 4; i++)
        {
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data[i]);
            address += 4;
        }
        current_settings = save_settings;
    } 
    HAL_FLASH_Lock();
}
void App_Settings_SetBrightness(uint8_t brightness)
{
    current_settings.brightness = brightness;
    App_Settings_Save(&current_settings);
}
void App_Settings_SetWatchFace(uint8_t watch_face)
{
    current_settings.watch_face = watch_face;
    App_Settings_Save(&current_settings);
}
void App_Settings_SetScreenTimeout(uint32_t timeout_ms)
{
    current_settings.screen_timeout_ms = timeout_ms;
    App_Settings_Save(&current_settings);
}

 
