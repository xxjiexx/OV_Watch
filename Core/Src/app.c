#include "app.h"

#include "app_backlight.h"
#include "app_settings.h"
#include "app_ui.h"
#include "key.h"
#include "lcd.h"
#include "lv_port_indev.h"
#include "lv_port_lcd_stm32.h"
#include "lvgl.h"
#include "touch.h"

#define APP_DEFAULT_SCREEN_TIMEOUT_MS 10000U

static uint8_t screenOn = 1;
static uint8_t ignoreWakeKey = 0;
static uint32_t lastActiveTime = 0;
static uint32_t screenTimeoutMs = APP_DEFAULT_SCREEN_TIMEOUT_MS;

static void App_RecordActivity(void);
static void App_TurnScreenOn(void);
static void App_TurnScreenOff(void);

static void App_RecordActivity(void)
{
    lastActiveTime = HAL_GetTick();
}

static void App_TurnScreenOn(void)
{
    screenOn = 1;
    LCD_DisplayOn();
    lv_obj_invalidate(lv_screen_active()); // 亮屏后强制 LVGL 重画一次，避免 LCD 恢复后画面还是旧状态。
    App_RecordActivity();
}

static void App_TurnScreenOff(void)
{
    screenOn = 0;
    LCD_DisplayOff();
}

void App_SetScreenTimeout(uint32_t timeoutMs)
{
    screenTimeoutMs = timeoutMs;
    App_RecordActivity(); // 修改背光时间也算一次用户操作，避免刚设置完立刻息屏。
}

void App_Init(void)
{
    App_Backlight_Init(); // 先启动背光 PWM，让屏幕上电后一直有可见背光。
    Key_Init();           // 按键模块使用轮询状态机，KEY 引脚统一整理成普通输入。

    LCD_Init();
    LCD_Fill(0x0000); // LVGL 接管前先清掉 LCD 控制器里的旧显存，避免残留上一帧画面。
    LCD_WaitDMA();

    Touch_Reset();

    App_Settings_Init();
    const AppSettings *settings = App_Settings_Get();
    App_Backlight_SetBrightness(settings->brightness);
    if(settings->screen_timeout_ms == 0)
    {
        App_SetScreenTimeout(APP_SCREEN_TIMEOUT_ALWAYS);
    }
    else
    {
        App_SetScreenTimeout(settings->screen_timeout_ms);
    }

    lv_init();
    lv_tick_set_cb(HAL_GetTick);
    lv_port_disp_init();
    lv_port_indev_init();

    App_UI_Init();

    lv_obj_invalidate(lv_screen_active()); // 标记整屏需要重画，保证第一次显示不是局部刷新。
    lv_refr_now(NULL);                     // 立即执行一次刷新，不等下一轮 lv_timer_handler。

    screenOn = 1;
    App_RecordActivity();
}

void App_Update(void)
{
    uint8_t keyEvent = Key_GetEvent();
    uint8_t touchPressed = 0;

    if ((screenOn == 0) && (keyEvent == KEY_EVENT_1_DOWN))
    {
        ignoreWakeKey = 1; // 用 KEY1 唤醒时，忽略这一次按下后续产生的短按/长按，避免刚亮屏就误触发返回。
        App_TurnScreenOn();
        return;
    }

    if (screenOn == 0)
    {
        return;
    }

    if (ignoreWakeKey != 0)
    {
        if ((keyEvent == KEY_EVENT_1_SHORT) || (keyEvent == KEY_EVENT_1_LONG))
        {
            ignoreWakeKey = 0;
        }
        return;
    }

    touchPressed = Touch_IsPressed();
    if ((keyEvent != KEY_EVENT_NONE) || (touchPressed != 0))
    {
        App_RecordActivity(); // 按键或触摸都算用户活动，滑动/按住时自动息屏计时会重新开始。
    }

    if (keyEvent != KEY_EVENT_NONE)
    {
        App_UI_HandleKeyevent(keyEvent); // 把按键事件交给 UI 层处理，KEY2 返回和 KEY1 长按回主页都在 UI 层决定。
    }

    lv_timer_handler();

    if ((screenTimeoutMs != APP_SCREEN_TIMEOUT_ALWAYS) && ((HAL_GetTick() - lastActiveTime) >= screenTimeoutMs))
    {
        App_TurnScreenOff();
    }
}
