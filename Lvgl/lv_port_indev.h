#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

void lv_port_indev_init(void);
void lv_port_indev_get_press_start(lv_point_t *point);

#ifdef __cplusplus
}
#endif

#endif
