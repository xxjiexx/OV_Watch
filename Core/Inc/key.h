#ifndef INC_KEY_H_
#define INC_KEY_H_

#include "main.h"

#define KEY_EVENT_NONE 0
#define KEY_EVENT_1_SHORT 1
#define KEY_EVENT_1_LONG 2
#define KEY_EVENT_2_SHORT 3
#define KEY_EVENT_2_LONG 4
#define KEY_EVENT_1_DOWN 5

void Key_Init(void);
uint8_t Key_GetEvent(void);


#endif
