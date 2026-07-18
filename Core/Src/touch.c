#include "main.h"
#include <stdlib.h>

static void TP_Delay()
{
    volatile uint16_t i;

    for (i = 0; i < 80; i++)
    {
        __NOP();
    }
}

void TP_I2C_Start()
{
    TP_SDA_High();
    TP_SCL_High();

    TP_Delay();

    TP_SDA_Low();

    TP_Delay();

    TP_SCL_Low();
}

void TP_I2C_Stop()
{
    TP_SDA_Low();
    TP_SCL_High();

    TP_Delay();

    TP_SDA_High();

    TP_Delay();
}

void TP_I2C_WriteBit(int data)
{
    TP_SCL_Low();
    if(data) TP_SDA_High();
    else TP_SDA_Low();

    TP_Delay();

    TP_SCL_High();

    TP_Delay();

    TP_SCL_Low();
}

void TP_I2C_WriteByte(int data)
{
    for(int i = 7; i >= 0; i--)
    {
        TP_SCL_Low();
        if((data >> i) & 1) TP_SDA_High();
        else TP_SDA_Low();

        TP_Delay();

        TP_SCL_High();

        TP_Delay();

        TP_SCL_Low();

    }
    
}

int TP_I2C_ReadBit()
{
    int value = 0;
    TP_SCL_Low();
    TP_SDA_High();

    TP_Delay();

    TP_SCL_High();

    TP_Delay();

    value = TP_SDA_Read();

    TP_SCL_Low();

    return value;
}

int TP_I2C_ReadByte()
{
    int data = 0;

    for(int i = 0; i < 8;i++)
    {
        TP_SCL_Low();
        TP_SDA_High();

        TP_Delay();

        TP_SCL_High();

        TP_Delay();

        data = data << 1;
        if(TP_SDA_Read()) data |= 1;
    
        TP_SCL_Low();
    }
    return data;
}

int TP_I2C_WaitAck()
{
    TP_SCL_Low();
    TP_SDA_High();

    TP_Delay();

    TP_SCL_High();
    
    if(TP_SDA_Read()) 
    {
        TP_Delay();
        TP_SCL_Low();
        return 0;
    }
    else 
    {
        TP_Delay();
        TP_SCL_Low();
        return 1;
    }    
}

void TP_I2C_Ack()
{
    TP_SCL_Low();
    TP_SDA_Low();

    TP_Delay();

    TP_SCL_High();

    TP_Delay();

    TP_SCL_Low();
    TP_SDA_High();
}

void TP_I2C_NAck()
{
    TP_SCL_Low();
    TP_SDA_High();

    TP_Delay();

    TP_SCL_High();

    TP_Delay();

    TP_SCL_Low();
}

void Touch_Reset()
{
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(GPIOA,GPIO_PIN_15,GPIO_PIN_SET);

    TP_SCL_High();
    TP_SDA_High();
    HAL_Delay(50);
}

uint8_t Touch_IsPressed()
{
    int finger_num = 0;

    finger_num = ReadReg(0x15, 0x02);

    if(finger_num > 0) return 1;
    else return 0;
}

int ReadReg(int Device_address, int Reg_address)  //读取寄存器
{
    int value = 0;

    TP_I2C_Start();

    TP_I2C_WriteByte(Device_address << 1);
    if(TP_I2C_WaitAck() == 0) 
    {
        TP_I2C_Stop();
        return 0;
    }

    TP_I2C_WriteByte(Reg_address);
    if(TP_I2C_WaitAck() == 0) 
    {
        TP_I2C_Stop();
        return 0;
    }

    TP_I2C_Start();

    TP_I2C_WriteByte((Device_address << 1) | 1);
    if(TP_I2C_WaitAck() == 0) 
    {
        TP_I2C_Stop();
        return 0;
    }

    value = TP_I2C_ReadByte();
    
    TP_I2C_NAck();
    TP_I2C_Stop();
    return value;
}

int Touch_GetXY(int *x, int *y)
{
    int x1,x2,y1,y2;

    if(Touch_IsPressed() != 1) return 0;

    x1 = ReadReg(0x15, 0x03);
    x2 = ReadReg(0x15, 0x04);
    y1 = ReadReg(0x15, 0x05);
    y2 = ReadReg(0x15, 0x06);
    *x = ((x1 & 0x0F) << 8) | x2;
    *y = ((y1 & 0x0F) << 8) | y2;

    return 1;
}

int Touch_GetGesture(void)
{
    int x = 0, y = 0;
    int dx = 0, dy = 0;
    int nowPressed = 0;
    static int lastPressed = 0;
    static int startX = 0, startY = 0;
    static int lastX = 0, lastY = 0;
    static uint32_t PressStartTime = 0;
    static int longPressTriggered = 0;

    nowPressed = Touch_GetXY(&x, &y);

    // 上一轮没按、这一轮按下：说明这是一次新触摸的开始，记录起点和开始时间。
    if((lastPressed == 0) && (nowPressed == 1))
    {
        PressStartTime = HAL_GetTick();
        longPressTriggered = 0;
        lastPressed = nowPressed;
        startX = x;
        startY = y;
        lastX = x;
        lastY = y;
        return TOUCH_EVENT_DOWN;
    }

    // 手指一直按着：检查是否已经超过长按时间，并且同一次长按只触发一次。
    if(nowPressed == 1)
    {
        if((HAL_GetTick() - PressStartTime) > 1000)
        {
            if(longPressTriggered == 0)
            {
                longPressTriggered = 1;
                lastPressed = nowPressed;
                return TOUCH_EVENT_LONG;
            }
        }
    }

    // 手指还按着：持续更新最后一次坐标，松手时用它和起点计算滑动方向。
    if(nowPressed == 1)
    {
        lastX = x;
        lastY = y;
    }

    // 按着但还没有触发长按：这就是普通移动事件，例如拖动或画线。
    if((nowPressed == 1) && (longPressTriggered == 0))
    {
        lastPressed = nowPressed;
        return TOUCH_EVENT_MOVE;
    }

    // 上一轮还按着、这一轮松开：说明一次触摸结束，在这里判断是否构成左右滑。
    if((lastPressed == 1) && (nowPressed == 0))
    {
        lastPressed = nowPressed;
        dx = lastX - startX;
        dy = lastY - startY;

        // 如果这次触摸已经触发过长按，松手时只结束状态，不再额外判断滑动。
        if(longPressTriggered == 1)
        {
            longPressTriggered = 0;
            return TOUCH_EVENT_UP;
        }

        longPressTriggered = 0;

        // 横向移动距离足够大，并且横向明显大于纵向：判断为右滑。
        if((dx > 50) && (abs(dx) > abs(dy)))
        {
            return TOUCH_SWIPE_RIGHT;
        }

        // 横向移动距离足够大，并且横向明显大于纵向：判断为左滑。
        if((dx < -50) && (abs(dx) > abs(dy)))
        {
            return TOUCH_SWIPE_LEFT;
        }

        // 松手了，但移动距离不够大：只是一次普通抬手事件。
        return TOUCH_EVENT_UP;
    }

    // 没有按下，也没有新的松手/滑动/长按事件。
    lastPressed = nowPressed;
    return TOUCH_EVENT_NONE;
}
