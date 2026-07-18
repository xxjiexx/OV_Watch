#ifndef APP_ANIM_FRAMES_H
#define APP_ANIM_FRAMES_H

#include <stdint.h>
#include "lvgl.h"

/* 控制中心动图帧数量：由 demo.gif 离线采样生成，运行时不再解析 GIF。 */
#define APP_CONTROL_ANIM_FRAME_COUNT 8

/* 控制中心动图每帧尺寸：缩小到 72x72，避免占用太多 Flash。 */
#define APP_CONTROL_ANIM_FRAME_WIDTH 72
#define APP_CONTROL_ANIM_FRAME_HEIGHT 72

/* 控制中心动图帧数组：每个元素都是一张 RGB565 图片，可以直接给 lv_image_set_src 使用。 */
extern const lv_image_dsc_t app_control_anim_frames[APP_CONTROL_ANIM_FRAME_COUNT];

#endif
