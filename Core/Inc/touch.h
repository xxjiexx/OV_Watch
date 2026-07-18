#include "main.h"

#ifndef INC_TOUCH_H_
#define INC_TOUCH_H_

#define TOUCH_EVENT_NONE 0
#define TOUCH_EVENT_DOWN 1
#define TOUCH_EVENT_MOVE 2
#define TOUCH_EVENT_UP 3
#define TOUCH_EVENT_LONG 4
#define TOUCH_SWIPE_LEFT 5
#define TOUCH_SWIPE_RIGHT 6

#define TP_SCL_High() HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_SET)
#define TP_SCL_Low() HAL_GPIO_WritePin(GPIOB,GPIO_PIN_6,GPIO_PIN_RESET)
#define TP_SDA_High() HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_SET) 
#define TP_SDA_Low() HAL_GPIO_WritePin(GPIOB,GPIO_PIN_4,GPIO_PIN_RESET)
#define TP_SDA_Read() HAL_GPIO_ReadPin(GPIOB,GPIO_PIN_4)

void TP_I2C_Start(void);
void TP_I2C_Stop(void);
void TP_I2C_WriteBit(int data);
void TP_I2C_WriteByte(int data);
int TP_I2C_ReadBit(void);
int TP_I2C_ReadByte(void);
int TP_I2C_WaitAck(void);
void TP_I2C_Ack(void);
void TP_I2C_NAck(void);

void Touch_Reset(void);
uint8_t Touch_IsPressed(void);
int ReadReg(int Device_address, int Reg_address);
int Touch_GetXY(int *x, int *y);
int Touch_GetGesture(void);

#endif
