#include "app_backlight.h"
#include "main.h"
#include "stm32f4xx_hal_tim.h"

void App_Backlight_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
    App_Backlight_SetBrightness(80);
}

void App_Backlight_SetBrightness(uint8_t percent)
{
    uint32_t period;
    uint32_t pulse;

    if(percent > 100)
    {
        percent = 100;
    }

    if(percent < 5)
    {
        percent = 5;
    }

    /* 外部传入的是 0~100 的百分比，这里统一换算成 TIM3 的 CCR 值。 */
    period = __HAL_TIM_GET_AUTORELOAD(&htim3);
    pulse = ((period + 1U) * percent) / 100U;

    if(pulse > period)
    {
        pulse = period;
    }

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, pulse);
}
