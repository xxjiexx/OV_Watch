#include "lv_port_lcd_stm32.h"

#include "lcd.h"
#include "spi.h"

#include <string.h>

#define LCD_HOR_RES 240
#define LCD_VER_RES 280
#define LCD_BUF_LINES 20
#define LV_PORT_DIAG_IGNORE_PX_MAP 0

static lv_display_t *lcd_disp;
static uint8_t lv_draw_buf_1[LCD_HOR_RES * LCD_BUF_LINES * 2];
static uint8_t lv_draw_buf_2[LCD_HOR_RES * LCD_BUF_LINES * 2];
static uint8_t lcd_line_buf[LCD_HOR_RES * 2];

static void lv_port_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);
static void lv_port_prepare_line(uint16_t width, uint16_t y, const uint8_t *px_map);

void lv_port_disp_init(void)
{
    lcd_disp = lv_display_create(LCD_HOR_RES, LCD_VER_RES);
    lv_display_set_flush_cb(lcd_disp, lv_port_flush_cb);
    lv_display_set_buffers(lcd_disp,
                           lv_draw_buf_1,
                           lv_draw_buf_2,
                           sizeof(lv_draw_buf_1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
}

static void lv_port_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;
    uint32_t area_stride = (uint32_t)(area->x2 - area->x1 + 1) * 2;
    uint32_t px_offset_x = 0;
    uint32_t px_offset_y = 0;

    if ((area->x2 < 0) || (area->y2 < 0) || (area->x1 >= LCD_HOR_RES) || (area->y1 >= LCD_VER_RES))
    {
        lv_display_flush_ready(disp);
        return;
    }

    if (x1 < 0)
    {
        px_offset_x = (uint32_t)(-x1);
        x1 = 0;
    }

    if (y1 < 0)
    {
        px_offset_y = (uint32_t)(-y1);
        y1 = 0;
    }

    if (x2 >= LCD_HOR_RES)
    {
        x2 = LCD_HOR_RES - 1;
    }

    if (y2 >= LCD_VER_RES)
    {
        y2 = LCD_VER_RES - 1;
    }

    int32_t width = x2 - x1 + 1;
    int32_t height = y2 - y1 + 1;

    if ((width <= 0) || (height <= 0))
    {
        lv_display_flush_ready(disp);
        return;
    }

    LCD_WaitDMA();
    LCD_SetWindow((uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2);
    LCD_WriteCmd(0x2C);
    LCD_DC_DATA();
    LCD_CS_LOW();

    for (int32_t row = 0; row < height; row++)
    {
        uint8_t *row_px = &px_map[(row + px_offset_y) * area_stride + px_offset_x * 2];

        /* 诊断模式会忽略 px_map，直接生成颜色；正常模式用 row_px 发送真实 LVGL 像素。 */
        lv_port_prepare_line((uint16_t)width, (uint16_t)(y1 + row), row_px);
        HAL_SPI_Transmit(&hspi1, lcd_line_buf, (uint16_t)(width * 2), 100);
    }

    LCD_CS_HIGH();

    lv_display_flush_ready(disp);
}

static void lv_port_prepare_line(uint16_t width, uint16_t y, const uint8_t *px_map)
{
    if (width > LCD_HOR_RES)
    {
        width = LCD_HOR_RES;
    }

#if LV_PORT_DIAG_IGNORE_PX_MAP
    LV_UNUSED(px_map);

    for (uint16_t i = 0; i < width; i++)
    {
        uint16_t color = 0x001F;

        if (y < 70)
        {
            color = 0x07E0;
        }
        else if (y >= 210)
        {
            color = 0xF800;
        }

        lcd_line_buf[i * 2] = (uint8_t)(color >> 8);
        lcd_line_buf[i * 2 + 1] = (uint8_t)(color & 0xFF);
    }
#else
    LV_UNUSED(y);

    for (uint16_t i = 0; i < width; i++)
    {
        /* LVGL 的 RGB565 在内存里是低字节在前，LCD 需要高字节先发。 */
        uint16_t color = (uint16_t)px_map[i * 2] | ((uint16_t)px_map[i * 2 + 1] << 8);

        lcd_line_buf[i * 2] = (uint8_t)(color >> 8);
        lcd_line_buf[i * 2 + 1] = (uint8_t)(color & 0xFF);
    }
#endif
}
