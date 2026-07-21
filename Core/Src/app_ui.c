/**
 * @file app_ui.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "app_backlight.h"
#include "app_time.h"
#include "lvgl.h"
#include "key.h"

#include "app.h"
#include "app_ui.h"
#include "app_sensor.h"
#include "app_anim_frames.h"
#include "lv_port_indev.h"
#include "rtc.h"
#include "stm32f4xx_hal_rtc.h"
#include "app_settings.h"

/*********************
 *      DEFINES
 *********************/
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 280
#define APP_BUTTON_MAX_COUNT        32
#define APP_BUTTON_MOVE_THRESHOLD   18
#define APP_BACK_GESTURE_EDGE_X     35

/**********************
 *      TYPEDEFS
 **********************/
typedef enum{
  APP_PAGE_WATCH,
  APP_PAGE_MENU,
  APP_PAGE_CONTROLCENTER,
  APP_PAGE_WATCH_SELECT,
  APP_PAGE_SETTING,
  APP_PAGE_ABOUT,
  APP_PAGE_SENSOR,
  APP_PAGE_BACKLIGHT,
  APP_PAGE_HEART,
  APP_PAGE_ALTITUDE,
  APP_PAGE_MOTION,
  APP_PAGE_COMPASS,
  APP_PAGE_WEATHER,
  APP_PAGE_DATETIME,
}app_page_t;

typedef enum{
  WATCH_FACE_COLOR,
  WATCH_FACE_SIMPLE,
}watch_face_t;

 typedef enum{
  MENU_BUTTON_SETTING,
  MENU_BUTTON_WATCH,
  MENU_BUTTON_ABOUT,
  MENU_BUTTON_SENSOR,
 }menu_button_t;

typedef enum{
  SETTING_BUTTON_BACKLIGHT,
  SETTING_BUTTON_DATETIME,
}setting_button_t;

typedef enum{
  DATETIME_BUTTON_SAVE,
  DATETIME_BUTTON_SWAP,
}datetime_button_t;

typedef enum{
  DATETIME_MODE_TIME,
  DATETIME_MODE_DATE,
}datetime_mode_t;

typedef enum{
  BACKLIGHT_TIMEOUT_5S,
  BACKLIGHT_TIMEOUT_10S,
  BACKLIGHT_TIMEOUT_30S,
  BACKLIGHT_TIMEOUT_ALWAYS,
}backlight_button_t;

typedef enum{
   SENSOR_HEART,
   SENSOR_ALTITUDE,
   SENSOR_COMPASS,
   SENSOR_MOTION,
   SENSOR_WEATHER,
}sensor_button_t;

typedef struct{
  lv_event_cb_t event_cb;
  uintptr_t id;
  lv_point_t press_point;
  uint8_t moved;
}app_button_ctx_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void Page_Watch(void);
static void watch_timer_cb(lv_timer_t *timer);
static int get_days_in_month(int year, int month);

static void ContorlCenter_Create(void);
static void Brightnessslider_Event(lv_event_t *e);
static void ControlAnim_Create(lv_obj_t *parent);
static void ControlAnim_TimerCb(lv_timer_t *timer);
static void ControlAnim_ApplyFrame(uint8_t frame_index);

static void Watch_Gesture_Event(lv_event_t *e);

static void Page_Menu_Create(void);
static void Menu_Button_Event(lv_event_t *e);

static void Page_Watch_Select_Create(void);
static void WatchFace_Set(watch_face_t face);
static void WatchFace_Refresh(void);
static void WatchFace_CreateColor(lv_obj_t *parent);
static void WatchFace_CreateSimpleDigital(lv_obj_t *parent);
static void WatchFace_Select_Event(lv_event_t *e);

static void App_HideAllPages(void);
static void App_GotoPage(app_page_t page);
static lv_obj_t *App_CreateButton(lv_obj_t *parent, const char *text, int x, int y, int width, int height, lv_event_cb_t event_cb, uintptr_t id);
static void App_ButtonRouter_Event(lv_event_t *e);
static uintptr_t App_ButtonGetId(lv_event_t *e);
static uint8_t App_IsEdgeBackGesture(void);

static void Page_Setting_Create(void);
static void Setting_Button_Event(lv_event_t *e);
static void Page_About_Create(void);
static void Page_Sensor_Create(void);
static void Page_Backlight_Create(void);
static void Backlight_Button_Event(lv_event_t *e);
static void Page_Datetime_Create(void);
static void Datetime_Button_Event(lv_event_t *e);


static void App_Toast_Create(void);
static void App_ShowToast(const char *text);

static void Sensor_Button_Event(lv_event_t *e);
static void Page_Heart_Create(void);
static void Page_Altitude_Create(void);
static void Page_Compass_Create(void);
static void Page_Motion_Create(void);
static void Page_Weather_Create(void);

static void SensorData_Refresh(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static lv_obj_t *time_label;
static lv_obj_t *date_label;
static lv_obj_t *day_arc;

static lv_obj_t *contorl_center;
static lv_obj_t *brightness;
static lv_obj_t *brightness_slider;
static lv_obj_t *control_anim_img;

static lv_obj_t *Page_Menu;
static lv_obj_t *Page_Watch_Select;
static lv_obj_t *Page_Watch_Root;

static app_page_t current_page = APP_PAGE_WATCH;

static watch_face_t current_watch_face = WATCH_FACE_COLOR;

static lv_obj_t *Page_Setting;
static lv_obj_t *Page_About;
static lv_obj_t *Page_Sensor;
static lv_obj_t *Page_Backlight;
static lv_obj_t *Page_Sensor_Compass;
static lv_obj_t *Page_Sensor_Altitude;
static lv_obj_t *Page_Sensor_Motion;
static lv_obj_t *Page_Sensor_Heart;
static lv_obj_t *Page_Sensor_Weather;
static lv_obj_t *Page_Datetime;


static lv_obj_t *app_toast;
static lv_obj_t *app_toast_label;

static lv_obj_t *Sensor_temp_label;
static lv_obj_t *Sensor_humi_label;
static lv_obj_t *Sensor_heart_label;
static lv_obj_t *Sensor_spo2_label;
static lv_obj_t *Sensor_alti_label;
static lv_obj_t *Sensor_press_label;
static lv_obj_t *Sensor_angle_label;
static lv_obj_t *Sensor_step_label;

static app_button_ctx_t app_button_ctx[APP_BUTTON_MAX_COUNT];
static uint8_t app_button_ctx_count;

static uint16_t hour = 12;
static uint16_t min = 38;
static uint16_t sec = 0;

static uint16_t year = 2026;
static uint16_t month = 7;
static uint16_t day = 11;

static lv_obj_t *hour_rollor;
static lv_obj_t *sec_rollor;
static lv_obj_t *min_rollor;
static lv_obj_t *year_rollor;
static lv_obj_t *mon_rollor;
static lv_obj_t *day_rollor;
static datetime_mode_t datetime_mode = DATETIME_MODE_TIME;

/* 滚轮选项用静态字符串保存，避免在创建函数里塞超长文本，也避免运行时重复生成占用 RAM。 */
static const char datetime_hour_options[] =
"00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n"
"12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23";

/* 分钟和秒都是 00~59，所以共用同一组选项。 */
static const char datetime_min_sec_options[] =
"00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n"
"10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n"
"20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n"
"30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n"
"40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n"
"50\n51\n52\n53\n54\n55\n56\n57\n58\n59";

/* 日期年份先给 2020~2035，范围够学习和演示使用，后面需要更长再扩展。 */
static const char datetime_year_options[] =
"2020\n2021\n2022\n2023\n2024\n2025\n2026\n2027\n"
"2028\n2029\n2030\n2031\n2032\n2033\n2034\n2035";

/* 月份固定 01~12。 */
static const char datetime_month_options[] =
"01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12";

/* 日期滚轮固定显示 01~31，保存时再检查这个月到底有没有这一天。 */
static const char datetime_day_options[] =
"01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n"
"11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n"
"21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31";


/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void App_UI_Init(void)
{
  const AppSettings *settings = App_Settings_Get();
  current_watch_face = settings->watch_face;
  lv_obj_add_event_cb(lv_screen_active(), Watch_Gesture_Event, LV_EVENT_GESTURE, NULL);
  Page_Watch();
}

void App_UI_HandleKeyevent(uint8_t KeyEvent)
{
  if(KeyEvent == KEY_EVENT_1_LONG)
  {
    App_GotoPage(APP_PAGE_WATCH); // KEY1 长按作为全局主页键，无论在哪个页面都回到主表盘。
    return;
  }

  if(KeyEvent == KEY_EVENT_1_SHORT)
  {
    switch (current_page) 
    {
      case APP_PAGE_SETTING:
      case APP_PAGE_ABOUT:
      case APP_PAGE_SENSOR:
      case APP_PAGE_WATCH_SELECT:
        App_GotoPage(APP_PAGE_MENU); // KEY2 短按返回上一级：二级入口页回到菜单。
        break;

      case APP_PAGE_BACKLIGHT:
      case APP_PAGE_DATETIME:
        App_GotoPage(APP_PAGE_SETTING); // 设置子页面返回设置页。
        break;

      case APP_PAGE_HEART:
      case APP_PAGE_ALTITUDE:
      case APP_PAGE_MOTION:
      case APP_PAGE_COMPASS:
      case APP_PAGE_WEATHER:
        App_GotoPage(APP_PAGE_SENSOR); // 传感器详情页返回传感器列表。
        break;

      case APP_PAGE_CONTROLCENTER:
      case APP_PAGE_MENU:
        App_GotoPage(APP_PAGE_WATCH); // 控制中心和主菜单再返回就是主表盘。
        break;

      default:
        break;
    }
  }
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void Page_Watch(void)
{
  Page_Watch_Root = lv_obj_create(lv_screen_active());
  lv_obj_set_size(Page_Watch_Root, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_opa(Page_Watch_Root, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(Page_Watch_Root, 0, 0);
  lv_obj_set_style_pad_all(Page_Watch_Root, 0, 0);
  lv_obj_clear_flag(Page_Watch_Root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_center(Page_Watch_Root);

  WatchFace_Refresh();
  lv_timer_create(watch_timer_cb, 1000, NULL);

  /* 开机只创建表盘本体，其它页面等用户第一次进入时再创建，降低 LVGL 启动阶段的内存峰值。 */
}

static void WatchFace_CreateColor(lv_obj_t *parent)
{
  lv_obj_set_style_bg_color(parent, lv_color_hex(0x120018), 0);
  lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0); // 表盘背景必须完全不透明，否则真屏会露出 LCD 旧显存。
  lv_obj_t *dial = lv_obj_create(parent);
  lv_obj_set_size(dial ,202, 202);
  lv_obj_set_style_radius(dial, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(dial, lv_color_hex(0x080510), 0);
  lv_obj_set_style_border_color(dial, lv_color_hex(0xFF2D55), 0);
  lv_obj_set_style_border_width(dial, 2, 0);
  lv_obj_set_style_shadow_width(dial, 0, 0); // STM32F411 RAM 较小，先关闭软件阴影，避免首帧只画出一部分。
  lv_obj_set_style_pad_all(dial, 0, 0);
  lv_obj_clear_flag(dial, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_center(dial);

  lv_obj_t *pink_dot = lv_obj_create(dial);
  lv_obj_set_size(pink_dot, 18, 18);
  lv_obj_set_style_radius(pink_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(pink_dot, lv_color_hex(0xFF2D55), 0);
  lv_obj_set_style_border_width(pink_dot, 0, 0);
  lv_obj_align(pink_dot, LV_ALIGN_CENTER, -58, -34);

  lv_obj_t *orange_dot = lv_obj_create(dial);
  lv_obj_set_size(orange_dot, 12, 12);
  lv_obj_set_style_radius(orange_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(orange_dot, lv_color_hex(0xFF9F0A), 0);
  lv_obj_set_style_border_width(orange_dot, 0, 0);
  lv_obj_align(orange_dot, LV_ALIGN_CENTER, 64, -42);

  lv_obj_t *green_dot = lv_obj_create(dial);
  lv_obj_set_size(green_dot, 10, 10);
  lv_obj_set_style_radius(green_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(green_dot, lv_color_hex(0x32D74B), 0);
  lv_obj_set_style_border_width(green_dot, 0, 0);
  lv_obj_align(green_dot, LV_ALIGN_CENTER, -70, 46);

  date_label = lv_label_create(dial);
  lv_label_set_text(date_label, "2026/07/11 SAT");
  lv_obj_set_style_text_font(date_label, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(date_label, lv_color_hex(0xFFCCF2), 0);
  lv_obj_align(date_label, LV_ALIGN_CENTER, 0, -48);

  lv_obj_t *button = lv_button_create(dial);
  lv_obj_set_size(button, 30, 30);
  lv_obj_set_style_radius(button, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(0xBF5AF2), 0);
  lv_obj_set_style_bg_color(button, lv_color_hex(0xFFFFFF), LV_STATE_PRESSED);
  lv_obj_set_style_border_width(button, 0, 0);
  lv_obj_set_style_shadow_width(button, 0, 0); // 小控件阴影也会走软件绘制，硬件版先用纯色和边框表达层次。
  lv_obj_center(button);

  time_label = lv_label_create(dial);
  lv_label_set_text(time_label,"12:38:00");
  lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_18, 0);
  lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 46);

  day_arc = lv_arc_create(dial);
  lv_obj_set_size(day_arc, 196, 196);
  lv_arc_set_range(day_arc, 0, 86400);
  lv_obj_remove_style(day_arc, NULL, LV_PART_KNOB);
  lv_obj_remove_flag(day_arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(day_arc, 8, LV_PART_MAIN);
  lv_obj_set_style_arc_width(day_arc, 10, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(day_arc, lv_color_hex(0x2B1435), LV_PART_MAIN);
  lv_obj_set_style_arc_color(day_arc, lv_color_hex(0x32D74B), LV_PART_INDICATOR);
  lv_obj_center(day_arc);
}

static void WatchFace_CreateSimpleDigital(lv_obj_t *parent)
{
  lv_obj_set_style_bg_color(parent, lv_color_hex(0x05070D), 0);
  lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0); // 切换到简洁表盘时也保证整屏背景盖住底图。
  lv_obj_t *card = lv_obj_create(parent);
  lv_obj_set_size(card, 214, 236);
  lv_obj_set_style_radius(card, 34, 0);
  lv_obj_set_style_bg_color(card, lv_color_hex(0x10131C), 0);
  lv_obj_set_style_border_color(card, lv_color_hex(0x2C3140), 0);
  lv_obj_set_style_border_width(card, 1, 0);
  lv_obj_set_style_shadow_width(card, 0, 0); // 简洁表盘的大卡片阴影同样耗 RAM，后续可改成多层边框模拟发光。
  lv_obj_set_style_pad_all(card, 0, 0);
  lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_center(card);

  lv_obj_t *accent = lv_obj_create(card);
  lv_obj_set_size(accent, 86, 8);
  lv_obj_set_style_radius(accent, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(accent, lv_color_hex(0x30D158), 0);
  lv_obj_set_style_border_width(accent, 0, 0);
  lv_obj_align(accent, LV_ALIGN_TOP_MID, 0, 20);

  date_label = lv_label_create(card);
  lv_label_set_text(date_label, "2026/07/11 SAT");
  lv_obj_set_style_text_font(date_label, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(date_label, lv_color_hex(0xA7F3D0), 0);
  lv_obj_align(date_label, LV_ALIGN_TOP_MID, 0, 48);

  time_label = lv_label_create(card);
  lv_label_set_text(time_label, "12:38:00");
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_44, 0);
  lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -4);

  lv_obj_t *status = lv_label_create(card);
  lv_label_set_text(status, "BAT 86%  |  STEP 1234");
  lv_obj_set_style_text_font(status, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(status, lv_color_hex(0x8E8E93), 0);
  lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -38);

  lv_obj_t *blue_dot = lv_obj_create(card);
  lv_obj_set_size(blue_dot, 12, 12);
  lv_obj_set_style_radius(blue_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(blue_dot, lv_color_hex(0x0A84FF), 0);
  lv_obj_set_style_border_width(blue_dot, 0, 0);
  lv_obj_align(blue_dot, LV_ALIGN_BOTTOM_MID, -64, -16);

  lv_obj_t *pink_dot = lv_obj_create(card);
  lv_obj_set_size(pink_dot, 12, 12);
  lv_obj_set_style_radius(pink_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(pink_dot, lv_color_hex(0xFF375F), 0);
  lv_obj_set_style_border_width(pink_dot, 0, 0);
  lv_obj_align(pink_dot, LV_ALIGN_BOTTOM_MID, 0, -16);

  lv_obj_t *orange_dot = lv_obj_create(card);
  lv_obj_set_size(orange_dot, 12, 12);
  lv_obj_set_style_radius(orange_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(orange_dot, lv_color_hex(0xFF9F0A), 0);
  lv_obj_set_style_border_width(orange_dot, 0, 0);
  lv_obj_align(orange_dot, LV_ALIGN_BOTTOM_MID, 64, -16);

  /* 简洁表盘没有 24 小时圆环，所以这里置空，定时器里会跳过圆环刷新。 */
  day_arc = NULL;
}

static void watch_timer_cb(lv_timer_t *timer)
{
  LV_UNUSED(timer);

  char buf[16];
  char datebuf[24];

  RTC_TimeTypeDef time = {0};
  RTC_DateTypeDef date = {0};

  HAL_RTC_GetTime(&hrtc, &time, RTC_FORMAT_BIN); // 读取 RTC 时间，结果会写入 time 结构体。
  HAL_RTC_GetDate(&hrtc, &date, RTC_FORMAT_BIN); // 读取 RTC 日期，读取时间后必须再读日期，RTC 影子寄存器才会更新。

  if(current_page != APP_PAGE_WATCH || time_label == NULL || date_label == NULL)
  {
    return;
  }

  int day_sec = time.Hours * 3600 + time.Minutes * 60 + time.Seconds; // 把当前时间换算成一天内经过的秒数，用来刷新 24 小时进度环。

  lv_snprintf(buf, sizeof(buf), "%02d:%02d:%02d", time.Hours, time.Minutes, time.Seconds);
  lv_snprintf(datebuf, sizeof(datebuf), "%04d/%02d/%02d %s", 2000 + date.Year, date.Month, date.Date,
    date.WeekDay == RTC_WEEKDAY_MONDAY ? "MON" :
    date.WeekDay == RTC_WEEKDAY_TUESDAY ? "TUE" :
    date.WeekDay == RTC_WEEKDAY_WEDNESDAY ? "WED" :
    date.WeekDay == RTC_WEEKDAY_THURSDAY ? "THU" :
    date.WeekDay == RTC_WEEKDAY_FRIDAY ? "FRI" :
    date.WeekDay == RTC_WEEKDAY_SATURDAY ? "SAT" : "SUN");
  lv_label_set_text(time_label, buf);
  lv_label_set_text(date_label, datebuf);

  if(day_arc != NULL)
  {
    lv_arc_set_value(day_arc, day_sec); // 圆环显示一天进度，不是只显示当前秒数。
  }
}

static int get_days_in_month(int year, int month)
{
  switch(month)
  {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      return 31; // 大月固定 31 天。

    case 4:
    case 6:
    case 9:
    case 11:
      return 30; // 小月固定 30 天。

    case 2:
      if(((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))
      {
        return 29; // 闰年 2 月有 29 天。
      }

      return 28; // 平年 2 月有 28 天。

    default:
      return 31; // 防御性返回：月份异常时避免日期校验直接崩掉。
  }
}

static void ContorlCenter_Create(void)
{
  if(contorl_center != NULL)
  {
    return; // 控制中心采用懒加载：已经创建过就直接复用，避免重复分配 LVGL 对象。
  }

  contorl_center = lv_obj_create(lv_screen_active());

  /* 控制中心占屏幕上方 3/4，高度跟随屏幕宏，方便以后移植到手表。 */
  lv_obj_set_size(contorl_center, SCREEN_WIDTH, SCREEN_HEIGHT * 3 / 4);
  lv_obj_set_style_bg_color(contorl_center, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(contorl_center, 0, 0);
  lv_obj_set_style_pad_all(contorl_center, 20, 10);
  lv_obj_align(contorl_center, LV_ALIGN_TOP_MID, 0, 0);

  lv_obj_remove_flag(contorl_center, LV_OBJ_FLAG_SCROLLABLE); /* 控制中心是固定面板，不允许滚动，避免出现滚动条。 */
  lv_obj_add_flag(contorl_center, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t * label = lv_label_create(contorl_center);
  lv_label_set_text(label , "ControlCenter");
  lv_obj_set_style_text_color(label, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 16);

  ControlAnim_Create(contorl_center);

  brightness = lv_label_create(contorl_center);
  lv_label_set_text(brightness, "brightness: 80%");
  lv_obj_set_style_text_color(brightness, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(brightness, &lv_font_montserrat_14, 0);

  brightness_slider = lv_slider_create(contorl_center);
  lv_obj_set_size(brightness_slider, 112, 24);
  lv_slider_set_range(brightness_slider, 0, 100);
  lv_slider_set_value(brightness_slider, 80, 0);
  lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0xE5E5EA), LV_PART_MAIN);
  lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0x007AFF), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0x000000), LV_PART_KNOB);
  lv_obj_set_style_radius(brightness_slider, LV_RADIUS_CIRCLE, LV_PART_MAIN); /* 轨道加粗，手指更容易点到。 */
  lv_obj_set_style_radius(brightness_slider, LV_RADIUS_CIRCLE, LV_PART_INDICATOR);
  lv_obj_set_style_width(brightness_slider, 32, LV_PART_KNOB);
  lv_obj_set_style_height(brightness_slider, 32, LV_PART_KNOB);
  lv_obj_set_style_radius(brightness_slider, LV_RADIUS_CIRCLE, LV_PART_KNOB);
  lv_obj_align(brightness_slider, LV_ALIGN_CENTER, 54, 8);
  lv_obj_align_to(brightness, brightness_slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 12);
  lv_obj_add_event_cb(brightness_slider, Brightnessslider_Event, LV_EVENT_VALUE_CHANGED, NULL);

}

static void ControlAnim_Create(lv_obj_t *parent)
{
  /* 控制中心左侧的动画外框：只作为白色卡片上的装饰容器，不参与滚动和点击。 */
  lv_obj_t *box = lv_obj_create(parent);
  lv_obj_set_size(box, 84, 84);
  lv_obj_set_style_radius(box, 28, 0);
  lv_obj_set_style_bg_color(box, lv_color_hex(0xF2F2F7), 0);
  lv_obj_set_style_border_width(box, 0, 0);
  lv_obj_set_style_pad_all(box, 0, 0);
  lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(box, LV_ALIGN_LEFT_MID, -10, 20);

  /* 动画本体：使用 lv_image 显示离线生成的 RGB565 图片帧。 */
  control_anim_img = lv_image_create(box);
  lv_obj_remove_flag(control_anim_img, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_center(control_anim_img);

  /* 先显示第 0 帧，避免控制中心第一次出现时图片区域为空。 */
  ControlAnim_ApplyFrame(0);
  /* 定时切换帧：这里不是 GIF 解码，只是在几张图片之间轮播，所以 RAM 压力更小。 */
  lv_timer_create(ControlAnim_TimerCb, 180, NULL);
}

static void ControlAnim_TimerCb(lv_timer_t *timer)
{
  LV_UNUSED(timer);

  static uint8_t frame_index = 0;

  if((contorl_center == NULL) || lv_obj_has_flag(contorl_center, LV_OBJ_FLAG_HIDDEN))
  {
    return;
  }

  frame_index = (frame_index + 1) % APP_CONTROL_ANIM_FRAME_COUNT;
  ControlAnim_ApplyFrame(frame_index);
}

static void ControlAnim_ApplyFrame(uint8_t frame_index)
{
  if(control_anim_img == NULL)
  {
    return;
  }

  /* 不解码 GIF，只在 8 张 RGB565 图片帧之间切换，省 RAM，也更稳定。 */
  lv_image_set_src(control_anim_img, &app_control_anim_frames[frame_index % APP_CONTROL_ANIM_FRAME_COUNT]);
  lv_obj_center(control_anim_img);
}

static void Brightnessslider_Event(lv_event_t *e)
{
  LV_UNUSED(e);

  int value = lv_slider_get_value(brightness_slider);

  App_Backlight_SetBrightness(value);

  char buf[32];
  lv_snprintf(buf, sizeof(buf), "brightness: %2d%%", value);

  lv_label_set_text(brightness, buf);
}
static void Watch_Gesture_Event(lv_event_t *e)
{
  LV_UNUSED(e);

  lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
  uint8_t edge_back = App_IsEdgeBackGesture();

  switch(current_page)
  {
    case APP_PAGE_WATCH:
      if(dir == LV_DIR_BOTTOM)
      {
        App_GotoPage(APP_PAGE_CONTROLCENTER);
      }
      else if(dir == LV_DIR_LEFT)
      {
        App_GotoPage(APP_PAGE_MENU);
      }
      break;

    case APP_PAGE_CONTROLCENTER:
      if(dir == LV_DIR_TOP)
      {
        App_GotoPage(APP_PAGE_WATCH);
      }
      break;

    case APP_PAGE_MENU:
      if(dir == LV_DIR_RIGHT && edge_back)
      {
        App_GotoPage(APP_PAGE_WATCH);
      }
      break;

    case APP_PAGE_WATCH_SELECT:
      if(dir == LV_DIR_RIGHT && edge_back)
      {
        App_GotoPage(APP_PAGE_MENU);
      }
      break;

    case APP_PAGE_SETTING:
    if(dir == LV_DIR_RIGHT && edge_back)
      {
        App_GotoPage(APP_PAGE_MENU);
      }
      break;

    case APP_PAGE_ABOUT:
    if(dir == LV_DIR_RIGHT && edge_back)
      {
        App_GotoPage(APP_PAGE_MENU);
      }
      break;

    case APP_PAGE_SENSOR:
    if(dir == LV_DIR_RIGHT && edge_back)
      {
        App_GotoPage(APP_PAGE_MENU);
      }
      break;

    case APP_PAGE_BACKLIGHT:
    if(dir == LV_DIR_RIGHT && edge_back)
      {
        App_GotoPage(APP_PAGE_SETTING);
      }
      break;

    case APP_PAGE_DATETIME:
    if(dir == LV_DIR_RIGHT && edge_back)
      {
        App_GotoPage(APP_PAGE_SETTING);
      }
      break;

    case APP_PAGE_HEART:
    if(dir == LV_DIR_RIGHT && edge_back)
      {
        App_GotoPage(APP_PAGE_SENSOR);
      }
      break;

    case APP_PAGE_ALTITUDE:
    if(dir == LV_DIR_RIGHT && edge_back)
      {
        App_GotoPage(APP_PAGE_SENSOR);
      }
      break;

    case APP_PAGE_COMPASS:
    if(dir == LV_DIR_RIGHT && edge_back)
      {
        App_GotoPage(APP_PAGE_SENSOR);
      }
      break;

    case APP_PAGE_MOTION:
    if(dir == LV_DIR_RIGHT && edge_back)
      {
        App_GotoPage(APP_PAGE_SENSOR);
      }
      break;

    case APP_PAGE_WEATHER:
    if(dir == LV_DIR_RIGHT && edge_back)
      {
        App_GotoPage(APP_PAGE_SENSOR);
      }
      break;

    default:
      break;
  }
}
static void Page_Menu_Create(void)
{
  Page_Menu = lv_obj_create(lv_screen_active());

  lv_obj_set_size(Page_Menu, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(Page_Menu, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(Page_Menu, 0, 0);
  lv_obj_set_style_pad_all(Page_Menu, 20, 0);
  lv_obj_center(Page_Menu);

  lv_obj_add_flag(Page_Menu, LV_OBJ_FLAG_HIDDEN);


  lv_obj_t *title = lv_label_create(Page_Menu);
  lv_label_set_text(title, "Menu");
  lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  App_CreateButton(Page_Menu, "Settings", -50, -40, 80, 48, Menu_Button_Event, MENU_BUTTON_SETTING);
  App_CreateButton(Page_Menu, "Watch",     50, -40, 80, 48, Menu_Button_Event, MENU_BUTTON_WATCH);
  App_CreateButton(Page_Menu, "About",    -50,  30, 80, 48, Menu_Button_Event, MENU_BUTTON_ABOUT);
  App_CreateButton(Page_Menu, "Sensor",    50,  30, 80, 48, Menu_Button_Event, MENU_BUTTON_SENSOR);
}

static lv_obj_t *App_CreateButton(lv_obj_t *parent, const char *text, int x, int y, int width, int height, lv_event_cb_t event_cb, uintptr_t id)
{
  lv_obj_t *btn = lv_button_create(parent);

  lv_obj_set_size(btn, width, height);
  lv_obj_align(btn, LV_ALIGN_CENTER, x, y);

  lv_obj_set_style_radius(btn, 14, 0);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0xF2F2F2), 0);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0xD1D1D1), LV_STATE_PRESSED);
  lv_obj_set_style_border_width(btn, 0, 0);

  /* 通用按钮只负责创建外观和绑定 ID，具体点击逻辑交给各页面自己的事件函数处理。 */
  if(app_button_ctx_count < APP_BUTTON_MAX_COUNT)
  {
    app_button_ctx_t *ctx = &app_button_ctx[app_button_ctx_count++];
    ctx->event_cb = event_cb;
    ctx->id = id;
    ctx->press_point.x = 0;
    ctx->press_point.y = 0;
    ctx->moved = 0;

    /* 按钮先经过统一路由器：滑动距离过大就不触发点击，避免右滑返回时误点按钮。 */
    lv_obj_add_event_cb(btn, App_ButtonRouter_Event, LV_EVENT_ALL, ctx);
  }

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text);
  lv_obj_set_style_text_color(label, lv_color_hex(0x000000), 0);
  lv_obj_center(label);

  return btn;
}

static void App_ButtonRouter_Event(lv_event_t *e)
{
  app_button_ctx_t *ctx = (app_button_ctx_t *)lv_event_get_user_data(e);
  lv_event_code_t code = lv_event_get_code(e);
  lv_indev_t *indev = lv_indev_active();
  lv_point_t point;

  if(ctx == NULL)
  {
    return;
  }

  if(code == LV_EVENT_PRESSED)
  {
    ctx->moved = 0;
    if(indev != NULL)
    {
      lv_indev_get_point(indev, &ctx->press_point);
    }
  }
  else if(code == LV_EVENT_PRESSING)
  {
    if(indev != NULL)
    {
      lv_indev_get_point(indev, &point);
      if(abs(point.x - ctx->press_point.x) > APP_BUTTON_MOVE_THRESHOLD ||
         abs(point.y - ctx->press_point.y) > APP_BUTTON_MOVE_THRESHOLD)
      {
        ctx->moved = 1;
      }
    }
  }
  else if(code == LV_EVENT_CLICKED)
  {
    if(ctx->moved == 0 && ctx->event_cb != NULL)
    {
      ctx->event_cb(e);
    }
  }
}

static uintptr_t App_ButtonGetId(lv_event_t *e)
{
  app_button_ctx_t *ctx = (app_button_ctx_t *)lv_event_get_user_data(e);

  if(ctx == NULL)
  {
    return 0;
  }

  return ctx->id;
}

static uint8_t App_IsEdgeBackGesture(void)
{
  lv_point_t start_point;

  lv_port_indev_get_press_start(&start_point);

  /* 返回手势限定从左边缘开始，普通按钮区域右滑就不会触发返回。 */
  return (start_point.x <= APP_BACK_GESTURE_EDGE_X) ? 1 : 0;
}

static void Menu_Button_Event(lv_event_t *e)
{
  menu_button_t button_id = (menu_button_t)App_ButtonGetId(e);

  switch(button_id)
  {
    case MENU_BUTTON_WATCH:
      App_GotoPage(APP_PAGE_WATCH_SELECT);
      break;

    case MENU_BUTTON_SETTING:
    App_GotoPage(APP_PAGE_SETTING);
      break;

    case MENU_BUTTON_ABOUT:
    App_GotoPage(APP_PAGE_ABOUT);
      break;

    case MENU_BUTTON_SENSOR:
    App_GotoPage(APP_PAGE_SENSOR);
      break;

    default:
      break;
  }
}

static void Page_Watch_Select_Create(void)
{
  Page_Watch_Select = lv_obj_create(lv_screen_active());
  lv_obj_set_size(Page_Watch_Select, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(Page_Watch_Select, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(Page_Watch_Select, 0, 0);
  lv_obj_set_style_pad_all(Page_Watch_Select, 20, 0);
  lv_obj_center(Page_Watch_Select);
  lv_obj_add_flag(Page_Watch_Select, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *title = lv_label_create(Page_Watch_Select);
  lv_label_set_text(title, "Watch Face");
  lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  App_CreateButton(Page_Watch_Select, "Color Dial",      0, -45, 200, 48, WatchFace_Select_Event, WATCH_FACE_COLOR);
  App_CreateButton(Page_Watch_Select, "Simple Digital",  0,  25, 200, 48, WatchFace_Select_Event, WATCH_FACE_SIMPLE);
}
static void WatchFace_Set(watch_face_t face)
{
  switch(face)
  {
    case WATCH_FACE_COLOR:
    case WATCH_FACE_SIMPLE:
      current_watch_face = face;
      App_Settings_SetWatchFace(face);
      WatchFace_Refresh();
      break;

    default:
      break;
  }
}
static void WatchFace_Refresh(void)
{
  if(Page_Watch_Root == NULL)
  {
    return;
  }

  /* 切换表盘前先清空旧控件，避免新旧表盘叠在一起。 */
  lv_obj_clean(Page_Watch_Root);
  time_label = NULL;
  date_label = NULL;
  day_arc = NULL;

  switch(current_watch_face)
  {
    case WATCH_FACE_COLOR:
      WatchFace_CreateColor(Page_Watch_Root);
      break;

    case WATCH_FACE_SIMPLE:
      WatchFace_CreateSimpleDigital(Page_Watch_Root);
      break;

    default:
      WatchFace_CreateColor(Page_Watch_Root);
      break;
  }
}
static void WatchFace_Select_Event(lv_event_t *e)
{
  watch_face_t face = (watch_face_t)App_ButtonGetId(e);

  switch(face)
  {
    case WATCH_FACE_COLOR:
      WatchFace_Set(WATCH_FACE_COLOR);
      break;

    case WATCH_FACE_SIMPLE:
      WatchFace_Set(WATCH_FACE_SIMPLE);
      break;

    default:
      break;
  }

  /* 选中表盘后回到主表盘页面，菜单和选择页都会隐藏。 */
  App_GotoPage(APP_PAGE_WATCH);
}

static void App_HideAllPages(void)
{
  if(Page_Menu != NULL)
  {
    lv_obj_add_flag(Page_Menu, LV_OBJ_FLAG_HIDDEN);
  }

  if(Page_Watch_Select != NULL)
  {
    lv_obj_add_flag(Page_Watch_Select, LV_OBJ_FLAG_HIDDEN);
  }

  if(contorl_center != NULL)
  {
    lv_obj_add_flag(contorl_center, LV_OBJ_FLAG_HIDDEN);
  }

  if(Page_Setting != NULL)
  {
    lv_obj_add_flag(Page_Setting, LV_OBJ_FLAG_HIDDEN);
  }

  if(Page_About != NULL)
  {
    lv_obj_add_flag(Page_About, LV_OBJ_FLAG_HIDDEN);
  }

  if(Page_Sensor != NULL)
  {
    lv_obj_add_flag(Page_Sensor, LV_OBJ_FLAG_HIDDEN);
  }

  if(Page_Backlight != NULL)
  {
    lv_obj_add_flag(Page_Backlight, LV_OBJ_FLAG_HIDDEN);
  }

  if(Page_Datetime != NULL)
  {
    lv_obj_add_flag(Page_Datetime, LV_OBJ_FLAG_HIDDEN); // 页面跳转前也要隐藏日期时间页，避免它盖住返回后的设置页。
  }

  if(Page_Sensor_Altitude != NULL)
  {
    lv_obj_add_flag(Page_Sensor_Altitude, LV_OBJ_FLAG_HIDDEN);
  }

  if(Page_Sensor_Compass != NULL)
  {
    lv_obj_add_flag(Page_Sensor_Compass, LV_OBJ_FLAG_HIDDEN);
  }

  if(Page_Sensor_Motion != NULL)
  {
    lv_obj_add_flag(Page_Sensor_Motion, LV_OBJ_FLAG_HIDDEN);
  }

  if(Page_Sensor_Heart != NULL)
  {
    lv_obj_add_flag(Page_Sensor_Heart, LV_OBJ_FLAG_HIDDEN);
  }

  if(Page_Sensor_Weather != NULL)
  {
    lv_obj_add_flag(Page_Sensor_Weather, LV_OBJ_FLAG_HIDDEN);
  }
}
static void App_GotoPage(app_page_t page)
{
  App_HideAllPages();

  switch(page)
  {
    case APP_PAGE_WATCH:
      break;

    case APP_PAGE_MENU:
      if(Page_Menu == NULL)
      {
        Page_Menu_Create(); // 第一次左滑进菜单时才创建菜单页面，避免开机一次性分配太多控件。
      }
      lv_obj_remove_flag(Page_Menu, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(Page_Menu);
      break;

    case APP_PAGE_CONTROLCENTER:
      if(contorl_center == NULL)
      {
        ContorlCenter_Create(); // 第一次下滑打开控制中心时再创建，减少开机阶段的 LVGL 内存压力。
      }
      lv_obj_remove_flag(contorl_center, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(contorl_center);
      break;

    case APP_PAGE_WATCH_SELECT:
      if(Page_Watch_Select == NULL)
      {
        Page_Watch_Select_Create(); // 表盘选择页不是开机必需页面，进入 Watch 菜单后再创建。
      }
      lv_obj_remove_flag(Page_Watch_Select, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(Page_Watch_Select);
      break;

    case APP_PAGE_SETTING:
      if(Page_Setting == NULL)
      {
        Page_Setting_Create(); // 设置页按需创建，减少主表盘启动时的 LVGL 对象数量。
      }
      lv_obj_remove_flag(Page_Setting, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(Page_Setting);
      break;

    case APP_PAGE_ABOUT:
      if(Page_About == NULL)
      {
        Page_About_Create(); // 关于页面只在用户进入时创建，开机不用提前占用 LVGL 堆。
      }
      lv_obj_remove_flag(Page_About, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(Page_About);
      break;

    case APP_PAGE_SENSOR:
      if(Page_Sensor == NULL)
      {
        Page_Sensor_Create(); // 传感器列表页按需创建，和传感器详情页保持同一种懒加载风格。
      }
      lv_obj_remove_flag(Page_Sensor, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(Page_Sensor);
      break;

    case APP_PAGE_BACKLIGHT:
      if(Page_Backlight == NULL)
      {
        Page_Backlight_Create(); // 背光时间设置页从设置入口进入时再创建，避免开机创建无关按钮。
      }
      lv_obj_remove_flag(Page_Backlight, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(Page_Backlight);
      break;

    case APP_PAGE_DATETIME:
      if(Page_Datetime == NULL)
      {
        Page_Datetime_Create(); // 背光时间设置页从设置入口进入时再创建，避免开机创建无关按钮。
      }
      lv_obj_remove_flag(Page_Datetime, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(Page_Datetime);
      break;

    case APP_PAGE_HEART:
      if(Page_Sensor_Heart == NULL)
      {
        Page_Heart_Create();
      }
      SensorData_Refresh();
      lv_obj_remove_flag(Page_Sensor_Heart, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(Page_Sensor_Heart);
      break;

    case APP_PAGE_MOTION:
      if(Page_Sensor_Motion == NULL)
      {
        Page_Motion_Create();
      }
      SensorData_Refresh();
      lv_obj_remove_flag(Page_Sensor_Motion, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(Page_Sensor_Motion);
      break;

    case APP_PAGE_COMPASS:
      if(Page_Sensor_Compass == NULL)
      {
        Page_Compass_Create();
      }
      SensorData_Refresh();
      lv_obj_remove_flag(Page_Sensor_Compass, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(Page_Sensor_Compass);
      break;
    case APP_PAGE_ALTITUDE:
      if(Page_Sensor_Altitude == NULL)
      {
        Page_Altitude_Create();
      }
      SensorData_Refresh();
      lv_obj_remove_flag(Page_Sensor_Altitude, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(Page_Sensor_Altitude);
      break;

    case APP_PAGE_WEATHER:
      if(Page_Sensor_Weather == NULL)
      {
        Page_Weather_Create();
      }
      SensorData_Refresh();
      lv_obj_remove_flag(Page_Sensor_Weather, LV_OBJ_FLAG_HIDDEN);
      lv_obj_move_foreground(Page_Sensor_Weather);
      break;

    default:
      page = APP_PAGE_WATCH;
      break;
  }

  current_page = page;
}

static void Page_Setting_Create(void)
{
  Page_Setting = lv_obj_create(lv_screen_active());
  lv_obj_set_size(Page_Setting , SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(Page_Setting, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(Page_Setting, 0, 0);
  lv_obj_set_style_pad_all(Page_Setting, 20, 0);
  lv_obj_center(Page_Setting);
  lv_obj_add_flag(Page_Setting, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *title = lv_label_create(Page_Setting);
  lv_label_set_text(title, "Setting");
  lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  App_CreateButton(Page_Setting, "Backlight Time", 0, -42, 200, 42, Setting_Button_Event, SETTING_BUTTON_BACKLIGHT);
  App_CreateButton(Page_Setting, "Date & Time", 0, 12, 200, 42, Setting_Button_Event, SETTING_BUTTON_DATETIME);
}

static void Setting_Button_Event(lv_event_t *e)
{
  setting_button_t button_id = (setting_button_t)App_ButtonGetId(e);

  switch(button_id)
  {
    case SETTING_BUTTON_BACKLIGHT:
      App_GotoPage(APP_PAGE_BACKLIGHT);
      break;

    case SETTING_BUTTON_DATETIME:
      App_GotoPage(APP_PAGE_DATETIME);
      break;

    default:
      break;
  }
}

static void Page_Sensor_Create(void)
{
  Page_Sensor = lv_obj_create(lv_screen_active());
  lv_obj_t *sensor_list;

  lv_obj_set_size(Page_Sensor , SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(Page_Sensor, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(Page_Sensor, 0, 0);
  lv_obj_set_style_pad_all(Page_Sensor, 20, 0);
  lv_obj_center(Page_Sensor);
  lv_obj_add_flag(Page_Sensor, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(Page_Sensor, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(Page_Sensor);
  lv_label_set_text(title, "Sensor");
  lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  sensor_list = lv_obj_create(Page_Sensor);
  lv_obj_set_size(sensor_list, 220, 190);
  lv_obj_align(sensor_list, LV_ALIGN_TOP_MID, 0, 62);
  lv_obj_set_style_bg_opa(sensor_list, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(sensor_list, 0, 0);
  lv_obj_set_style_pad_all(sensor_list, 0, 0);
  lv_obj_set_style_pad_row(sensor_list, 8, 0);
  lv_obj_set_scroll_dir(sensor_list, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(sensor_list, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_flex_flow(sensor_list, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(sensor_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_scrollbar_mode(sensor_list,  LV_SCROLLBAR_MODE_OFF);

  /* 传感器按钮交给 flex 从上到下自动排列，避免用负坐标导致滚动范围计算不准。 */
  App_CreateButton(sensor_list, "Motion", 0, 0, 200, 42, Sensor_Button_Event, SENSOR_MOTION);
  App_CreateButton(sensor_list, "Heart", 0, 0, 200, 42, Sensor_Button_Event, SENSOR_HEART);
  App_CreateButton(sensor_list, "Altitude", 0, 0, 200, 42, Sensor_Button_Event, SENSOR_ALTITUDE);
  App_CreateButton(sensor_list, "Compass", 0, 0, 200, 42, Sensor_Button_Event, SENSOR_COMPASS);
  App_CreateButton(sensor_list, "Weather", 0, 0, 200, 42, Sensor_Button_Event, SENSOR_WEATHER);
}

static void Page_Backlight_Create(void)
{
  Page_Backlight = lv_obj_create(lv_screen_active());
  lv_obj_set_size(Page_Backlight , SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(Page_Backlight, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(Page_Backlight, 0, 0);
  lv_obj_set_style_pad_all(Page_Backlight, 20, 0);
  lv_obj_center(Page_Backlight);
  lv_obj_add_flag(Page_Backlight, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(Page_Backlight, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(Page_Backlight);
  lv_label_set_text(title, "Backlight Time");
  lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  App_CreateButton(Page_Backlight, "5 seconds",   0, -52, 190, 42, Backlight_Button_Event, BACKLIGHT_TIMEOUT_5S);
  App_CreateButton(Page_Backlight, "10 seconds",  0,  -4, 190, 42, Backlight_Button_Event, BACKLIGHT_TIMEOUT_10S);
  App_CreateButton(Page_Backlight, "30 seconds",  0,  44, 190, 42, Backlight_Button_Event, BACKLIGHT_TIMEOUT_30S);
  App_CreateButton(Page_Backlight, "Always On",   0,  92, 190, 42, Backlight_Button_Event, BACKLIGHT_TIMEOUT_ALWAYS);
}

static void Backlight_Button_Event(lv_event_t *e)
{
  backlight_button_t button_id = (backlight_button_t)App_ButtonGetId(e);

  switch(button_id)
  {
    case BACKLIGHT_TIMEOUT_5S:
      App_SetScreenTimeout(5000U);
      App_Settings_SetScreenTimeout(5000);
      App_ShowToast("5 seconds");
      break;

    case BACKLIGHT_TIMEOUT_10S:
      App_SetScreenTimeout(10000U);
      App_Settings_SetScreenTimeout(10000);
      App_ShowToast("10 seconds");
      break;

    case BACKLIGHT_TIMEOUT_30S:
      App_SetScreenTimeout(30000U);
      App_Settings_SetScreenTimeout(30000);
      App_ShowToast("30 seconds");
      break;

    case BACKLIGHT_TIMEOUT_ALWAYS:
      App_SetScreenTimeout(APP_SCREEN_TIMEOUT_ALWAYS);
      App_Settings_SetScreenTimeout(0);
      App_ShowToast("Always On");
      break;

    default:
      break;
  }
}

static void Page_Datetime_Create(void)
{
  Page_Datetime = lv_obj_create(lv_screen_active());
  lv_obj_set_size(Page_Datetime , SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(Page_Datetime, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(Page_Datetime, 0, 0);
  lv_obj_set_style_pad_all(Page_Datetime, 20, 0);
  lv_obj_center(Page_Datetime);
  lv_obj_add_flag(Page_Datetime, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(Page_Datetime, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(Page_Datetime);
  lv_label_set_text(title, "Date & Time");
  lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  hour_rollor = lv_roller_create(Page_Datetime);
  lv_roller_set_options(hour_rollor, datetime_hour_options,  LV_ROLLER_MODE_INFINITE);
  lv_obj_set_size(hour_rollor, 60, 160);
  lv_obj_align(hour_rollor, LV_ALIGN_CENTER, -65, 0);
  lv_roller_set_selected(hour_rollor, hour, LV_ANIM_OFF); // 进入设置页时，小时滚轮默认停在当前表盘时间。

  min_rollor = lv_roller_create(Page_Datetime);
  lv_roller_set_options(min_rollor, datetime_min_sec_options,  LV_ROLLER_MODE_INFINITE);
  lv_obj_set_size(min_rollor, 60, 160);
  lv_obj_align(min_rollor, LV_ALIGN_CENTER, 0, 0);
  lv_roller_set_selected(min_rollor, min, LV_ANIM_OFF); // 进入设置页时，分钟滚轮默认停在当前表盘时间。

  sec_rollor = lv_roller_create(Page_Datetime);
  lv_roller_set_options(sec_rollor, datetime_min_sec_options,  LV_ROLLER_MODE_INFINITE);
  lv_obj_set_size(sec_rollor, 60, 160);
  lv_obj_align(sec_rollor, LV_ALIGN_CENTER, 65, 0);
  lv_roller_set_selected(sec_rollor, sec, LV_ANIM_OFF); // 进入设置页时，秒滚轮默认停在当前表盘时间。

  year_rollor = lv_roller_create(Page_Datetime);
  lv_roller_set_options(year_rollor, datetime_year_options,  LV_ROLLER_MODE_INFINITE);
  lv_obj_set_size(year_rollor, 70, 160);
  lv_obj_align(year_rollor, LV_ALIGN_CENTER, -65, 0);
  lv_roller_set_selected(year_rollor, year - 2020U, LV_ANIM_OFF); // 年份选项从 2020 开始，所以当前年要减去 2020。
  lv_obj_add_flag(year_rollor, LV_OBJ_FLAG_HIDDEN);

  mon_rollor = lv_roller_create(Page_Datetime);
  lv_roller_set_options(mon_rollor, datetime_month_options,  LV_ROLLER_MODE_INFINITE);
  lv_obj_set_size(mon_rollor, 60, 160);
  lv_obj_align(mon_rollor, LV_ALIGN_CENTER, 0, 0);
  lv_roller_set_selected(mon_rollor, month - 1U, LV_ANIM_OFF); // 月份显示 01~12，roller 下标从 0 开始。
  lv_obj_add_flag(mon_rollor, LV_OBJ_FLAG_HIDDEN);

  day_rollor = lv_roller_create(Page_Datetime);
  lv_roller_set_options(day_rollor, datetime_day_options,  LV_ROLLER_MODE_INFINITE);
  lv_obj_set_size(day_rollor, 60, 160);
  lv_obj_align(day_rollor, LV_ALIGN_CENTER, 65, 0);
  lv_roller_set_selected(day_rollor, day - 1U, LV_ANIM_OFF); // 日期显示 01~31，roller 下标从 0 开始。
  lv_obj_add_flag(day_rollor, LV_OBJ_FLAG_HIDDEN);

  App_CreateButton(Page_Datetime, "Save", -50, 120,  70,  40, Datetime_Button_Event, DATETIME_BUTTON_SAVE);
  App_CreateButton(Page_Datetime, "SWAP", 50, 120,  70,  40, Datetime_Button_Event, DATETIME_BUTTON_SWAP);

}

static void Datetime_Button_Event(lv_event_t *e)
{
  datetime_button_t button_id = (datetime_button_t)App_ButtonGetId(e);
  uint16_t selected_hour;
  uint16_t selected_min;
  uint16_t selected_sec;
  uint16_t selected_year;
  uint16_t selected_month;
  uint16_t selected_day;

  switch(button_id)
  {
    case DATETIME_BUTTON_SAVE:
      if(datetime_mode == DATETIME_MODE_TIME)
      {
        selected_hour = lv_roller_get_selected(hour_rollor);
        selected_min = lv_roller_get_selected(min_rollor);
        selected_sec = lv_roller_get_selected(sec_rollor);
        App_ShowToast("Time saved");
        hour = selected_hour;
        min = selected_min;
        sec = selected_sec;
      }
      else
      {
        selected_year = 2020U + lv_roller_get_selected(year_rollor);
        selected_month = 1U + lv_roller_get_selected(mon_rollor);
        selected_day = 1U + lv_roller_get_selected(day_rollor);

        if(selected_day > (uint16_t)get_days_in_month(selected_year, selected_month))
        {
          App_ShowToast("Invalid date"); // 例如 2 月 31 日不合法，直接提示，不修改当前日期。
          return;
        }

        year = selected_year;
        month = selected_month;
        day = selected_day;
        App_ShowToast("Date saved");
      }
      App_Time_Set(year, month, day, hour, min, sec);
      break;

    case DATETIME_BUTTON_SWAP:
      if(datetime_mode == DATETIME_MODE_TIME)
      {
        datetime_mode = DATETIME_MODE_DATE;
        lv_obj_add_flag(hour_rollor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(min_rollor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(sec_rollor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(year_rollor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(mon_rollor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(day_rollor, LV_OBJ_FLAG_HIDDEN);
      }
      else
      {
        datetime_mode = DATETIME_MODE_TIME;
        lv_obj_remove_flag(hour_rollor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(min_rollor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(sec_rollor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(year_rollor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(mon_rollor, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(day_rollor, LV_OBJ_FLAG_HIDDEN);
      }
      break;

    default:
      break;
  }
}

static void Page_About_Create(void)
{
  Page_About = lv_obj_create(lv_screen_active());
  lv_obj_set_size(Page_About , SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(Page_About, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(Page_About, 0, 0);
  lv_obj_set_style_pad_all(Page_About, 20, 0);
  lv_obj_center(Page_About);
  lv_obj_add_flag(Page_About, LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(Page_About, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_t *title = lv_label_create(Page_About);
  lv_label_set_text(title, "About");
  lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  lv_obj_t *infor = lv_label_create(Page_About);
  lv_label_set_text(infor, "OV-Watch\nSTM32F411CEU6\nby LVGL9.5\nby FreeRtos\nAuthor:TwentySeven!");
  lv_obj_set_style_text_color(infor, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(infor, &lv_font_montserrat_16, 0);
  lv_obj_align(infor, LV_ALIGN_CENTER, 0, -10);
}

static void App_Toast_Create(void)
{
  app_toast = lv_obj_create(lv_screen_active());
  lv_obj_set_size(app_toast, 160, 30);
  lv_obj_set_style_radius(app_toast, 15, 0);
  lv_obj_set_style_bg_color(app_toast, lv_color_hex(0xFFFFFFF), 0);
  lv_obj_set_style_border_width(app_toast, 0, 0);
  lv_obj_set_style_pad_all(app_toast, 0, 0);
  lv_obj_align(app_toast, LV_ALIGN_TOP_MID, 0, 8);
  lv_obj_remove_flag(app_toast, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(app_toast, LV_OBJ_FLAG_HIDDEN);

  app_toast_label = lv_label_create(app_toast);
  lv_label_set_text(app_toast_label, "");
  lv_obj_set_style_text_color(app_toast_label, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(app_toast_label, &lv_font_montserrat_14, 0);
  lv_obj_center(app_toast_label);
  lv_obj_set_style_opa(app_toast, LV_OPA_COVER, 0);
}
static void App_ShowToast(const char *text)
{
  if(app_toast == NULL)
  {
    App_Toast_Create(); // Toast 只有在真正需要提示时才创建，避免开机阶段提前占用 LVGL 对象内存。
  }

  if(app_toast == NULL || app_toast_label == NULL)
  {
    return;
  }

  lv_label_set_text(app_toast_label, text);
  lv_obj_set_style_opa(app_toast, LV_OPA_COVER, 0);
  lv_obj_remove_flag(app_toast, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(app_toast);

  /* 使用 LVGL 自带淡出动画：短暂显示后逐渐消失。 */
  lv_obj_fade_out(app_toast, 400, 500);
}

static void Sensor_Button_Event(lv_event_t *e)
{
  sensor_button_t button_id = (sensor_button_t)App_ButtonGetId(e);

  switch(button_id)
  {
    case SENSOR_ALTITUDE:
    App_GotoPage(APP_PAGE_ALTITUDE);
      break;

    case SENSOR_COMPASS:
    App_GotoPage(APP_PAGE_COMPASS);
      break;

    case SENSOR_MOTION:
    App_GotoPage(APP_PAGE_MOTION);
      break;

    case SENSOR_HEART:
    App_GotoPage(APP_PAGE_HEART);
      break;

    case SENSOR_WEATHER:
    App_GotoPage(APP_PAGE_WEATHER);
      break;

    default:
      break;
  }
}

static void Page_Heart_Create(void)
{
  Page_Sensor_Heart = lv_obj_create(lv_screen_active());
  lv_obj_set_size(Page_Sensor_Heart , SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(Page_Sensor_Heart, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(Page_Sensor_Heart, 0, 0);
  lv_obj_set_style_pad_all(Page_Sensor_Heart, 20, 0);
  lv_obj_center(Page_Sensor_Heart);
  lv_obj_add_flag(Page_Sensor_Heart, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *title = lv_label_create(Page_Sensor_Heart);
  lv_label_set_text(title, "Heart");
  lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  Sensor_heart_label = lv_label_create(Page_Sensor_Heart);
  lv_label_set_text_fmt(Sensor_heart_label, "HeartRate: %d", Sensor_data.heart_rate);
  lv_obj_set_style_text_color(Sensor_heart_label, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(Sensor_heart_label, &lv_font_montserrat_16, 0);
  lv_obj_align(Sensor_heart_label, LV_ALIGN_CENTER, 0, -20);

  Sensor_spo2_label = lv_label_create(Page_Sensor_Heart);
  lv_label_set_text_fmt(Sensor_spo2_label, "SpO2: %d %%", Sensor_data.spo2);
  lv_obj_set_style_text_color(Sensor_spo2_label, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(Sensor_spo2_label, &lv_font_montserrat_16, 0);
  lv_obj_align(Sensor_spo2_label, LV_ALIGN_CENTER, 0, 20);
}
static void Page_Altitude_Create(void)
{
  Page_Sensor_Altitude = lv_obj_create(lv_screen_active());
  lv_obj_set_size(Page_Sensor_Altitude , SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(Page_Sensor_Altitude, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(Page_Sensor_Altitude, 0, 0);
  lv_obj_set_style_pad_all(Page_Sensor_Altitude, 20, 0);
  lv_obj_center(Page_Sensor_Altitude);
  lv_obj_add_flag(Page_Sensor_Altitude, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *title = lv_label_create(Page_Sensor_Altitude);
  lv_label_set_text(title, "Altitude");
  lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  Sensor_alti_label = lv_label_create(Page_Sensor_Altitude);
  lv_label_set_text_fmt(Sensor_alti_label, "Altitude: %.1f m", Sensor_data.altitude);
  lv_obj_set_style_text_color(Sensor_alti_label, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(Sensor_alti_label, &lv_font_montserrat_16, 0);
  lv_obj_align(Sensor_alti_label, LV_ALIGN_CENTER, 0, -20);

  Sensor_press_label = lv_label_create(Page_Sensor_Altitude);
  lv_label_set_text_fmt(Sensor_press_label, "Pressure: %.1f hPa", Sensor_data.pressure);
  lv_obj_set_style_text_color(Sensor_press_label, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(Sensor_press_label, &lv_font_montserrat_16, 0);
  lv_obj_align(Sensor_press_label, LV_ALIGN_CENTER, 0, 20);
}
static void Page_Compass_Create(void)
{
  Page_Sensor_Compass = lv_obj_create(lv_screen_active());
  lv_obj_set_size(Page_Sensor_Compass , SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(Page_Sensor_Compass, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(Page_Sensor_Compass, 0, 0);
  lv_obj_set_style_pad_all(Page_Sensor_Compass, 20, 0);
  lv_obj_center(Page_Sensor_Compass);
  lv_obj_add_flag(Page_Sensor_Compass, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *title = lv_label_create(Page_Sensor_Compass);
  lv_label_set_text(title, "Compass");
  lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  Sensor_angle_label = lv_label_create(Page_Sensor_Compass);
  lv_label_set_text_fmt(Sensor_angle_label, "Angle: %.1f deg", Sensor_data.angle);
  lv_obj_set_style_text_color(Sensor_angle_label, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(Sensor_angle_label, &lv_font_montserrat_16, 0);
  lv_obj_align(Sensor_angle_label, LV_ALIGN_CENTER, 0, -20);
}
static void Page_Motion_Create(void)
{
  Page_Sensor_Motion = lv_obj_create(lv_screen_active());
  lv_obj_set_size(Page_Sensor_Motion , SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(Page_Sensor_Motion, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(Page_Sensor_Motion, 0, 0);
  lv_obj_set_style_pad_all(Page_Sensor_Motion, 20, 0);
  lv_obj_center(Page_Sensor_Motion);
  lv_obj_add_flag(Page_Sensor_Motion, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *title = lv_label_create(Page_Sensor_Motion);
  lv_label_set_text(title, "Motion");
  lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  Sensor_step_label = lv_label_create(Page_Sensor_Motion);
  lv_label_set_text_fmt(Sensor_step_label, "Step: %d", Sensor_data.step);
  lv_obj_set_style_text_color(Sensor_step_label, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(Sensor_step_label, &lv_font_montserrat_16, 0);
  lv_obj_align(Sensor_step_label, LV_ALIGN_CENTER, 0, -20);
}
static void Page_Weather_Create(void)
{
  Page_Sensor_Weather = lv_obj_create(lv_screen_active());
  lv_obj_set_size(Page_Sensor_Weather , SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(Page_Sensor_Weather, lv_color_hex(0xFFFFFF), 0);
  lv_obj_set_style_border_width(Page_Sensor_Weather, 0, 0);
  lv_obj_set_style_pad_all(Page_Sensor_Weather, 20, 0);
  lv_obj_center(Page_Sensor_Weather);
  lv_obj_add_flag(Page_Sensor_Weather, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t *title = lv_label_create(Page_Sensor_Weather);
  lv_label_set_text(title, "Weather");
  lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

  Sensor_temp_label = lv_label_create(Page_Sensor_Weather);
  lv_label_set_text_fmt(Sensor_temp_label, "Temperature: %.1f C", Sensor_data.temperature);
  lv_obj_set_style_text_color(Sensor_temp_label, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(Sensor_temp_label, &lv_font_montserrat_16, 0);
  lv_obj_align(Sensor_temp_label, LV_ALIGN_CENTER, 0, -20);

  Sensor_humi_label = lv_label_create(Page_Sensor_Weather);
  lv_label_set_text_fmt(Sensor_humi_label, "Humidity: %.1f %%", Sensor_data.humidity);
  lv_obj_set_style_text_color(Sensor_humi_label, lv_color_hex(0x000000), 0);
  lv_obj_set_style_text_font(Sensor_humi_label, &lv_font_montserrat_16, 0);
  lv_obj_align(Sensor_humi_label, LV_ALIGN_CENTER, 0, 20);
}

static void SensorData_Refresh(void)
{
  App_Sensor_Read();

  if(Sensor_temp_label != NULL) lv_label_set_text_fmt(Sensor_temp_label, "Temperature: %.1f C", Sensor_data.temperature);
  if(Sensor_humi_label != NULL) lv_label_set_text_fmt(Sensor_humi_label, "Humidity: %.1f %%", Sensor_data.humidity);
  if(Sensor_alti_label != NULL) lv_label_set_text_fmt(Sensor_alti_label, "Altitude: %.1f m", Sensor_data.altitude);
  if(Sensor_press_label != NULL) lv_label_set_text_fmt(Sensor_press_label, "Pressure: %.1f hPa", Sensor_data.pressure);
  if(Sensor_angle_label != NULL) lv_label_set_text_fmt(Sensor_angle_label, "Angle: %.1f deg", Sensor_data.angle);
  if(Sensor_step_label != NULL) lv_label_set_text_fmt(Sensor_step_label, "Step: %d", Sensor_data.step);
  if(Sensor_heart_label != NULL) lv_label_set_text_fmt(Sensor_heart_label, "HeartRate: %d", Sensor_data.heart_rate);
  if(Sensor_spo2_label != NULL) lv_label_set_text_fmt(Sensor_spo2_label, "SpO2: %d %%", Sensor_data.spo2);
}
