/*
 * lcd.h
 *
 *  Created on: 2026年5月25日
 *      Author: LXJ
 */

#include "main.h"

#ifndef INC_LCD_H_
#define INC_LCD_H_

#define LCD_DC_DATA() HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_SET) //DC选择发送 指令 还是 数据
#define LCD_DC_CMD() HAL_GPIO_WritePin(GPIOB,GPIO_PIN_9,GPIO_PIN_RESET)
#define LCD_CS_LOW() HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_RESET) //CS片选 low为打开通信 high为关闭通信
#define LCD_CS_HIGH() HAL_GPIO_WritePin(GPIOB,GPIO_PIN_8,GPIO_PIN_SET)
#define LCD_RST_LOW() HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_RESET) //RST复位 low为重启屏幕 high为正常运行
#define LCD_RST_HIGH() HAL_GPIO_WritePin(GPIOB,GPIO_PIN_7,GPIO_PIN_SET)

void LCD_RST(void);
void LCD_WriteCmd(uint8_t cmd);
void LCD_WriteData(uint8_t *data, uint16_t len);
void LCD_WriteByte(uint8_t data);
void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void LCD_Init();
void LCD_DisplayOn(void);
void LCD_DisplayOff(void);
uint8_t LCD_IsDMABusy(void);
void LCD_WaitDMA(void);
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi1);
void LCD_DMA_TxDone();

void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void LCD_FillArea(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_DrawHLine(uint16_t x1, uint16_t x2, uint16_t y, uint16_t color);
void LCD_DrawVLine(uint16_t y1, uint16_t y2, uint16_t x, uint16_t color);
void LCD_FillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void LCD_Fill(uint16_t color);

#endif /* INC_LCD_H_ */
