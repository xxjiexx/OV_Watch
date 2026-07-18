#include "lv_port_indev.h"

#include "touch.h"

static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data);

static lv_indev_t *touch_indev;
static lv_point_t press_start_point;
static uint8_t was_pressed;

void lv_port_indev_init(void)
{
    touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, touchpad_read);
    lv_indev_set_gesture_min_distance(touch_indev, 30);
    lv_indev_set_gesture_min_velocity(touch_indev, 3);
}

void lv_port_indev_get_press_start(lv_point_t *point)
{
    if (point == NULL)
    {
        return;
    }

    *point = press_start_point;
}

static void touchpad_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    LV_UNUSED(indev);

    static int last_x = 0;
    static int last_y = 0;
    int x = 0;
    int y = 0;

    if (Touch_GetXY(&x, &y) != 0)
    {
        if (x < 0) x = 0;
        if (x > 239) x = 239;
        if (y < 0) y = 0;
        if (y > 279) y = 279;

        last_x = x;
        last_y = y;
        if (was_pressed == 0)
        {
            /* 记录本次触摸刚按下的位置，用来判断是不是从屏幕边缘开始滑动。 */
            press_start_point.x = x;
            press_start_point.y = y;
            was_pressed = 1;
        }
        data->state = LV_INDEV_STATE_PRESSED;
    }
    else
    {
        was_pressed = 0;
        data->state = LV_INDEV_STATE_RELEASED;
    }

    data->point.x = last_x;
    data->point.y = last_y;
}
