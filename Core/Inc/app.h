#ifndef INC_APP_H_
#define INC_APP_H_

#include <stdint.h>

#define APP_SCREEN_TIMEOUT_ALWAYS 0U

void App_Init(void);
void App_Update(void);
void App_SetScreenTimeout(uint32_t timeoutMs);

#endif
