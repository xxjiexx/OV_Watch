#include "key.h"

#define KEY_LONG_PRESS_MS 1000
#define KEY1_ACTIVE_LEVEL GPIO_PIN_RESET
#define KEY1_RELEASE_LEVEL GPIO_PIN_SET
#define KEY2_ACTIVE_LEVEL GPIO_PIN_SET
#define KEY2_RELEASE_LEVEL GPIO_PIN_RESET

#define KEY_STATE_NONE 0
#define KEY_STATE_KEY1_PRESSED 1
#define KEY_STATE_KEY2_PRESSED 2
#define KEY_STATE_KEY1_LONG_REPORTED 3
#define KEY_STATE_KEY2_LONG_REPORTED 4

void Key_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* 当前按键使用轮询方式读取，不使用 EXTI 中断，避免按键抖动触发中断干扰主循环刷新。 */
    HAL_NVIC_DisableIRQ(EXTI9_5_IRQn);
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_5);

    /* KEY1：PA5，按下为低电平，所以配置成普通输入上拉。 */
    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* KEY2：PA4，按下为高电平，所以配置成普通输入下拉。 */
    GPIO_InitStruct.Pin = GPIO_PIN_4;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

uint8_t Key_GetEvent(void)
{
    static uint32_t pressStartTime = 0;
    static uint8_t keyState = KEY_STATE_NONE;
    uint32_t now = HAL_GetTick();
    GPIO_PinState key1Level = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
    GPIO_PinState key2Level = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4);

    if (keyState == KEY_STATE_NONE)
    {
        if (key1Level == KEY1_ACTIVE_LEVEL)
        {
            keyState = KEY_STATE_KEY1_PRESSED;
            pressStartTime = now;    // 记录 KEY1 按下时间，后面用来判断短按还是长按。
            return KEY_EVENT_1_DOWN; // 按下瞬间上报一次，息屏时可以立刻亮屏。
        }

        if (key2Level == KEY2_ACTIVE_LEVEL)
        {
            keyState = KEY_STATE_KEY2_PRESSED;
            pressStartTime = now;    // 记录 KEY2 按下时间，后面用来判断短按还是长按。
            return KEY_EVENT_NONE;
        }

        return KEY_EVENT_NONE;
    }

    if (keyState == KEY_STATE_KEY1_PRESSED)
    {
        if (key1Level == KEY1_ACTIVE_LEVEL)
        {
            if ((now - pressStartTime) >= KEY_LONG_PRESS_MS)
            {
                keyState = KEY_STATE_KEY1_LONG_REPORTED; // 长按已上报直接写进状态，避免再额外维护 longReported 标志。
                return KEY_EVENT_1_LONG;
            }

            return KEY_EVENT_NONE;
        }

        if (key1Level == KEY1_RELEASE_LEVEL)
        {
            keyState = KEY_STATE_NONE;
            return KEY_EVENT_1_SHORT;
        }
    }

    if (keyState == KEY_STATE_KEY1_LONG_REPORTED)
    {
        if (key1Level == KEY1_RELEASE_LEVEL)
        {
            keyState = KEY_STATE_NONE; // 长按已经上报过，松手时只复位状态，不再补一个短按。
        }

        return KEY_EVENT_NONE;
    }

    if (keyState == KEY_STATE_KEY2_PRESSED)
    {
        if (key2Level == KEY2_ACTIVE_LEVEL)
        {
            if ((now - pressStartTime) >= KEY_LONG_PRESS_MS)
            {
                keyState = KEY_STATE_KEY2_LONG_REPORTED; // KEY2 长按同样用状态表示，保持 KEY1/KEY2 逻辑一致。
                return KEY_EVENT_2_LONG;
            }

            return KEY_EVENT_NONE;
        }

        if (key2Level == KEY2_RELEASE_LEVEL)
        {
            keyState = KEY_STATE_NONE;
            return KEY_EVENT_2_SHORT;
        }
    }

    if (keyState == KEY_STATE_KEY2_LONG_REPORTED)
    {
        if (key2Level == KEY2_RELEASE_LEVEL)
        {
            keyState = KEY_STATE_NONE; // 长按已经上报过，松手时只复位状态，不再补一个短按。
        }

        return KEY_EVENT_NONE;
    }

    keyState = KEY_STATE_NONE; // 防御性复位：如果状态异常，下一轮从空闲状态重新开始。
    return KEY_EVENT_NONE;
}
