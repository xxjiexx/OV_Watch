/*
 * lcd.c
 *
 *  Created on: 2026-06-25
 *      Author: LXJ
 */
#include "main.h"

static uint8_t lineBuf[240 * 2];        // 一行 LCD 数据：240 个像素，每个 RGB565 像素占 2 字节。
static uint16_t fillColor = 0xFFFF;     // 当前整屏填充颜色，DMA 回调继续发下一行时会用它。
static uint16_t dmaline = 0;            // 记录当前这一帧已经发送了多少行。
static uint16_t dmaWidth = 0;           // 本次 DMA 区域每一行有多少个像素，局部刷新时不一定是 240。
static uint16_t dmaTotalLines = 0;      // 本次 DMA 区域一共要发送多少行，局部刷新时不一定是 280。
static volatile uint8_t lcdDmaBusy = 0; // DMA 是异步发送，这个标志防止上一帧没发完又启动新一帧。

static void LCD_PrepareLine(uint16_t color, uint16_t pixelCount)
{
    uint8_t hi = color >> 8;    // LCD 要求 RGB565 高字节先发，例如红色 0xF800 要先发 F8 再发 00。
    uint8_t lo = color & 0xFF;  // 不要直接 DMA uint16_t 数组：STM32 是小端序，字节顺序会反。
                                //将uint16_t 分成两个uint8_t来发送

    if (pixelCount > 240)
    {
        pixelCount = 240; // lineBuf 最多只放一整行 240 个像素，防止传入错误宽度导致数组越界。
    }

    for (uint16_t i = 0; i < pixelCount; i++)
    {
        lineBuf[i * 2] = hi;      // 当前像素的高字节。
        lineBuf[i * 2 + 1] = lo;  // 当前像素的低字节。
    }
}

void LCD_RST(void)
{
    LCD_RST_LOW();   // 拉低复位脚，让 LCD 控制器从确定状态重新启动。
    HAL_Delay(50);

    LCD_RST_HIGH();  // 释放复位脚，LCD 控制器需要一点时间才能接收命令。
    HAL_Delay(120);
}

void LCD_WriteCmd(uint8_t cmd)
{
    LCD_DC_CMD();    // DC 拉低表示接下来 SPI 发的是命令。
    LCD_CS_LOW();    // CS 拉低表示选中 LCD，开始本次 SPI 通信。
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    LCD_CS_HIGH();   // CS 拉高，结束这次命令传输。
}

void LCD_WriteData(uint8_t *data, uint16_t len)
{
    LCD_DC_DATA();   // DC 拉高表示接下来 SPI 发的是参数数据或像素数据。
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi1, data, len, 100);
    LCD_CS_HIGH();
}

void LCD_WriteByte(uint8_t data)
{
    LCD_DC_DATA();
    LCD_CS_LOW();
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);
    LCD_CS_HIGH();
}

void LCD_Init(void)
{
    uint8_t data;

    LCD_RST();       // 先硬件复位，避免 LCD 保留上一次运行时的状态。

    LCD_WriteCmd(0x11); // 退出睡眠模式。
    HAL_Delay(120);

    data = 0x55;        // 0x55 表示 RGB565 模式：16 位颜色，每个像素 2 字节。
    LCD_WriteCmd(0x3A); // COLMOD：设置像素格式。
    LCD_WriteData(&data, 1);

    data = 0x00;        // 基础扫描方向；如果画面镜像或旋转，通常改这里。
    LCD_WriteCmd(0x36); // MADCTL：设置显存访问方向。
    LCD_WriteData(&data, 1);

    LCD_WriteCmd(0x21); // 开启显示反色；很多小 IPS 屏需要这个命令颜色才正常。

    LCD_WriteCmd(0x29); // 打开显示。
    HAL_Delay(20);
}

void LCD_DisplayOn(void)
{
    LCD_WaitDMA();      // 等 DMA 刷屏结束后再发命令，避免 SPI 正在发像素时被打断。
    LCD_WriteCmd(0x29); // 0x29 是 Display ON，用来恢复 LCD 显示输出。
    HAL_Delay(20);
}

void LCD_DisplayOff(void)
{
    LCD_WaitDMA();      // 关显示前也要等 DMA 空闲，避免命令和像素数据混在一起。
    LCD_WriteCmd(0x28); // 0x28 是 Display OFF，只关闭显示输出，不一定切断背光电源。
    HAL_Delay(20);
}

void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t data[4];

    y1 += 20; // 这个 240x280 屏有纵向显存偏移，逻辑 y=0 对应控制器内部 y=20。
    y2 += 20;

    LCD_WriteCmd(0x2A); // 设置列地址，也就是 x 的起点和终点。

    data[0] = x1 >> 8;
    data[1] = x1 & 0xFF;
    data[2] = x2 >> 8;
    data[3] = x2 & 0xFF;
    LCD_WriteData(data, 4);

    LCD_WriteCmd(0x2B); // 设置行地址，也就是 y 的起点和终点。

    data[0] = y1 >> 8;
    data[1] = y1 & 0xFF;
    data[2] = y2 >> 8;
    data[3] = y2 & 0xFF;
    LCD_WriteData(data, 4);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        LCD_DMA_TxDone(); // HAL 在一行 DMA 发完后调用这里，我们在这里继续发下一行。
    }
}

void LCD_DMA_TxDone(void)
{
    dmaline++; // 刚刚已经发完一行 dmaWidth 像素的数据。

    if (dmaline < dmaTotalLines)
    {
        LCD_PrepareLine(fillColor, dmaWidth); // 准备下一行；填充区域内每一行颜色相同。

        if (HAL_SPI_Transmit_DMA(&hspi1, lineBuf, dmaWidth * 2) != HAL_OK)
        {
            dmaline = 0;            // 如果下一次 DMA 启动失败，就中止当前这一帧。
            dmaWidth = 0;
            dmaTotalLines = 0;
            LCD_CS_HIGH();
            lcdDmaBusy = 0;
        }
    }
    else
    {
        dmaline = 0;                // 本次区域的所有行已经全部发完。
        dmaWidth = 0;
        dmaTotalLines = 0;
        LCD_CS_HIGH();              // 结束这一次像素数据传输。
        lcdDmaBusy = 0;             // 标记 DMA 空闲，下一帧现在可以开始。
    }
}

uint8_t LCD_IsDMABusy(void)
{
    return lcdDmaBusy; // 给外部代码查询 DMA 是否还在忙，不会阻塞等待。
}

void LCD_WaitDMA(void)
{
    while (lcdDmaBusy)
    {
        HAL_Delay(1); // 阻塞等待，学习和简单测试时最直观。
    }
}

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    uint8_t data[2];

    while (lcdDmaBusy)
    {
        HAL_Delay(1); // 如果前面有 DMA 刷屏或画图，先等它发完，避免 SPI 总线冲突。
    }

    LCD_SetWindow(x, y, x, y); // 设置单像素窗口，接下来写入的 RGB565 只会落到这一个点。

    LCD_WriteCmd(0x2C); // 写显存命令。

    data[0] = color >> 8;   // 单像素只需要 2 字节，高字节先发。
    data[1] = color & 0xFF; // 低字节后发，不用 DMA，避免“小数据启动 DMA 反而更慢”。

    LCD_WriteData(data, 2);
}

void LCD_FillArea(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    uint16_t temp;

    while (lcdDmaBusy)
    {
        HAL_Delay(1); // 等上一帧 DMA 完全发完，才能安全复用 lineBuf。
    }

    if (x2 < x1)
    {
        temp = x1;
        x1 = x2;
        x2 = temp; // 允许调用时把左右坐标写反，先交换回来，避免 uint16_t 宽度下溢。
    }

    if (y2 < y1)
    {
        temp = y1;
        y1 = y2;
        y2 = temp; // 允许调用时把上下坐标写反，先交换回来。
    }

    if (x1 > 239)
    {
        x1 = 239; // 屏幕逻辑宽度是 0~239，超出范围就压到边界。
    }

    if (x2 > 239)
    {
        x2 = 239;
    }

    if (y1 > 279)
    {
        y1 = 279; // 屏幕逻辑高度是 0~279，超出范围就压到边界。
    }

    if (y2 > 279)
    {
        y2 = 279;
    }

    fillColor = color; // 回调函数会用这个颜色继续发送后面的每一行。
    dmaline = 0;       // 新的一帧从第 0 行开始发送。
    dmaWidth = x2 - x1 + 1;      // 只发送窗口内一行的像素数量，不再固定发送 240 个像素。
    dmaTotalLines = y2 - y1 + 1; // 只发送窗口内的行数，不再固定发送 280 行。
    lcdDmaBusy = 1;    // DMA 启动后会立刻返回，所以先标记为忙。

    LCD_SetWindow(x1, y1, x2, y2);
    LCD_WriteCmd(0x2C);            // 写显存命令：后面发的字节都是像素数据。
    LCD_DC_DATA();                 // 像素数据阶段保持 DC 为高。
    LCD_CS_LOW();                  // 整帧 DMA 发送期间保持 CS 为低，不中断这次显存写入。

    LCD_PrepareLine(fillColor, dmaWidth); // 按 LCD 需要的高字节在前格式，准备第一行数据。

    if (HAL_SPI_Transmit_DMA(&hspi1, lineBuf, dmaWidth * 2) != HAL_OK)
    {
        LCD_CS_HIGH();             // 如果 DMA 启动失败，释放 CS，避免 LCD 一直被选中。
        dmaline = 0;
        dmaWidth = 0;
        dmaTotalLines = 0;
        lcdDmaBusy = 0;
    }
}
void LCD_DrawHLine(uint16_t x1, uint16_t x2, uint16_t y, uint16_t color)
{
    LCD_FillArea(x1, y, x2, y, color);
}

void LCD_DrawVLine(uint16_t y1, uint16_t y2, uint16_t x, uint16_t color)
{
    LCD_FillArea(x, y1, x, y2, color);
}

void LCD_FillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color)
{
    LCD_FillArea(x1, y1, x2, y2, color);
}
void LCD_Fill(uint16_t color)
{
    LCD_FillArea(0, 0, 239, 279, color);
}
