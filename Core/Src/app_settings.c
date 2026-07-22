#include "app_settings.h"
#include "main.h"
#include "stm32f4xx_hal_flash.h"
#include "stm32f4xx_hal_flash_ex.h"
#include <stdint.h>

static const AppSettings default_settings = {
    .magic = APP_SETTINGS_MAGIC,
    .watch_face = 0,
    .brightness = 80,
    .screen_timeout_ms = 10000,
    .checksum = 0,
};

static AppSettings current_settings;

static uint32_t App_Settings_Checksum(const AppSettings *settings)
{
    uint32_t sum = 0;
    const uint8_t *data = (const uint8_t *)settings;

    // checksum 字段本身不参与校验，否则每次计算都会把旧校验值也算进去。
    for(uint32_t i = 0; i < sizeof(AppSettings) - sizeof(settings->checksum); i++)
    {
        sum += data[i];
    }

    return sum;
}

void App_Settings_Init(void)
{
    const AppSettings *flash_settings = (const AppSettings *)APP_SETTINGS_FLASH_ADDR;

    if(flash_settings->magic == APP_SETTINGS_MAGIC &&
       App_Settings_Checksum(flash_settings) == flash_settings->checksum)
    {
        current_settings = *flash_settings; // Flash 里的设置合法，复制到 RAM，后续运行都读 RAM。
    }
    else
    {
        current_settings = default_settings; // Flash 没保存过或数据损坏时，使用默认设置。
    }
}

const AppSettings *App_Settings_Get(void)
{
    return &current_settings;
}

uint8_t App_Settings_Save(const AppSettings *settings)
{
    AppSettings save_settings = *settings;
    FLASH_EraseInitTypeDef erase = {0};
    uint32_t sector_error = 0;
    uint32_t address = APP_SETTINGS_FLASH_ADDR;
    const uint32_t *data = (const uint32_t *)&save_settings;

    save_settings.magic = APP_SETTINGS_MAGIC;
    save_settings.checksum = App_Settings_Checksum(&save_settings);

    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = FLASH_SECTOR_7;
    erase.NbSectors = 1;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    HAL_FLASH_Unlock();

    // 写 Flash 前必须先擦除整个 sector，因为 Flash 只能把 1 写成 0，不能直接把 0 写回 1。
    if(HAL_FLASHEx_Erase(&erase, &sector_error) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return 0;
    }

    // 按 32 位 word 写入，AppSettings 的大小目前正好是 4 字节对齐。
    for(uint32_t i = 0; i < sizeof(save_settings) / 4U; i++)
    {
        if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data[i]) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return 0;
        }

        address += 4U;
    }

    HAL_FLASH_Lock();

    const AppSettings *flash_settings = (const AppSettings *)APP_SETTINGS_FLASH_ADDR;

    // 读回校验：只有 Flash 里真实读出的 magic 和 checksum 都正确，才认为保存成功。
    if(flash_settings->magic == APP_SETTINGS_MAGIC &&
       App_Settings_Checksum(flash_settings) == flash_settings->checksum)
    {
        current_settings = *flash_settings; // 校验成功后，以 Flash 读回的数据作为当前运行设置。
        return 1;
    }

    return 0;
}

uint8_t App_Settings_SetBrightness(uint8_t brightness)
{
    current_settings.brightness = brightness;
    return App_Settings_Save(&current_settings); // 返回 Flash 保存结果，方便 UI 判断是否保存成功。
}

uint8_t App_Settings_SetWatchFace(uint8_t watch_face)
{
    current_settings.watch_face = watch_face;
    return App_Settings_Save(&current_settings); // 返回 Flash 保存结果，方便 UI 判断是否保存成功。
}

uint8_t App_Settings_SetScreenTimeout(uint32_t timeout_ms)
{
    current_settings.screen_timeout_ms = timeout_ms;
    return App_Settings_Save(&current_settings); // 返回 Flash 保存结果，方便 UI 判断是否保存成功。
}
