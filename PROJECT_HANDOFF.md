# OV-Watch 项目交接记录

更新时间：2026-07-15

这份文档是给新的 Codex / 新对话接手用的。新会话请先读这里，再读关键源码，不要只凭聊天记忆继续改。

## 1. 用户背景与协作要求

- 用户是电子信息专业学生，即将大三，正在自主完成 STM32 智能手表项目，同时学习嵌入式、LVGL、RTOS、Linux/就业相关能力。
- 用户目标不是“能跑就行”，而是尽快做出一个像工程项目的作品，同时真正学懂关键代码。
- 用户希望 Codex 像项目搭档和老师：先读实际文件，再解释问题，再在用户明确要求时改代码。
- 用户不喜欢临时 demo 式做法，更偏好以后不用大改的工程化方案。
- 用户明确要求：除非他说“帮我改/开始工作/你来写/修改”，否则问“为什么/看看/啥情况/下一步”时先汇报，不要擅自改。
- 例外：漏分号、函数名拼错、枚举名拼错、明显 include 错误这类简单语法错误可以直接修，但要说明修了什么。
- 代码注释尽量写中文。后续新增或修改代码也要加明确中文注释。
- 每次改代码后最好说明：改了哪些文件、添加/修改/删除了哪些函数、为什么这么改、是否编译验证。
- 用户正在学习，解释要白话，但不要把方案降级成“学习阶段随便写”。应优先给真实工程里也能继续扩展的结构。

## 2. 重要路径

- STM32 工程：`C:\Users\TwentySeven\Desktop\OV_Watch`
- Windows LVGL 模拟工程：`C:\Users\TwentySeven\Desktop\LVGL\lv_port_pc_vscode-master`
- 项目资料网页：`https://no-chicken.com/content/OV-Watch`
- 当前主要开发集中在 STM32 工程 `OV_Watch`，LVGL 已经移植回 STM32 工程。

## 3. 开发环境

- 用户使用 VS Code 写代码、编译、调试、烧录。
- 使用 STM32CubeMX / STM32CubeIDE 配置引脚和生成 HAL 基础工程。
- 调试器使用 ST-LINK。
- CMake + Ninja 构建，VS Code task 会编译 `build/Debug/OV_Watch.elf`。
- 烧录使用 STM32CubeProgrammer CLI 或 VS Code 任务。
- 当前编译命令：

```powershell
& 'C:/Users/TwentySeven/AppData/Local/stm32cube/bundles/cmake/4.3.1+st.1/bin/cmake.exe' --build --preset Debug
```

## 4. 硬件和外设理解

- MCU：STM32F411 系列。
- LCD：SPI + DMA，逻辑分辨率 `240 x 280`。
- 当前 LCD 坐标范围：`x = 0~239`，`y = 0~279`。
- LCD 控制脚：
  - `PB9`: LCD DC
  - `PB8`: LCD CS
  - `PB7`: LCD RST
  - `PB0`: LCD_BL，已配置为 TIM3_CH3 PWM，用于背光亮度。
- SPI：
  - `PB3`: SPI1_SCK
  - `PB5`: SPI1_MOSI
- 触摸：
  - `TPCLK`: `PB6`
  - `TPSDA`: `PB4`
  - `TPRST`: `PA15`
- 按键：
  - `PA5`: KEY1，按下为低电平。
  - `PA4`: KEY2，当前代码按高电平为按下处理。
- 传感器规划：
  - `MPU6050`: 姿态/运动/步数。
  - `EM7028`: 心率/血氧。
  - `SPL06-001`: 气压/海拔。
  - `LSM303DLHC`: 指南针。
  - `AHT21`: 温湿度。

## 5. 当前源码结构

- `Core/Src/main.c`: CubeMX 主入口，初始化外设后调用 `App_Init()`，主循环调用 `App_Update()`。
- `Core/Src/app.c`: 应用主循环，初始化背光、按键、LCD、触摸、LVGL display/indev port，处理息屏/唤醒、触摸活动、LVGL timer。
- `Core/Src/app_ui.c`: 当前 UI 主文件，包含页面、表盘、菜单、控制中心、设置、背光、日期时间、传感器页面。
- `Core/Src/app_backlight.c`: 背光 PWM 初始化和亮度设置。
- `Core/Src/key.c`: 按键轮询状态机，已重写为 `keyState` 模式。
- `Core/Src/lcd.c`: ST7789 类屏幕底层驱动、窗口、RGB565 字节序处理、DMA 区域填充。
- `Lvgl/porting/lv_port_lcd_stm32.c`: LVGL display flush 到 LCD 的 porting。
- `Lvgl/porting/lv_port_indev.c`: LVGL 输入设备 porting，接触摸。

## 6. 已完成的重要功能

### LCD / LVGL 显示

- 早期花屏问题已解决。
- 根因是 RGB565 16 位颜色和 SPI 字节序。LCD 需要高字节先发，STM32 小端直接发 `uint16_t` 会反。
- 当前 `LCD_PrepareLine()` 将 `uint16_t color` 拆成高字节和低字节写入 `uint8_t` 缓冲。
- LVGL 移植后曾出现“上方五分之一正常、下方花屏/三色块/黑屏”等问题，最终通过对照原 LCD 驱动链路修复 flush/窗口/刷新逻辑。
- 当前 LVGL 可以在真机显示表盘、页面、控制中心等。

### 触摸

- 触摸读坐标、滑动、长按等基础功能已经能工作。
- LVGL 输入设备已接入触摸。
- 全局手势挂在 `lv_screen_active()`：

```c
lv_obj_add_event_cb(lv_screen_active(), Watch_Gesture_Event, LV_EVENT_GESTURE, NULL);
```

- 注意：所有页面的手势都会进 `Watch_Gesture_Event()`，具体行为由 `current_page` 决定。

### 按键

- 之前黑屏 bug 已定位并修复。
- 证据链：
  - LCD、LVGL flush、背光、GPIO 读取都正常。
  - 旧 `Key_GetEvent()` 的 `longReported = 1` 路径会触发黑屏。
  - 改为集中 `keyState` 状态机后，按键读取不再导致黑屏。
- 当前 `key.c` 使用状态：
  - `KEY_STATE_NONE`
  - `KEY_STATE_KEY1_PRESSED`
  - `KEY_STATE_KEY1_LONG_REPORTED`
  - `KEY_STATE_KEY2_PRESSED`
  - `KEY_STATE_KEY2_LONG_REPORTED`
- `Key_Init()` 会把 PA5/PA4 配成普通输入轮询，禁用 PA5 EXTI。

### 背光 / 息屏

- PB0 已做 TIM3_CH3 PWM 背光亮度。
- 控制中心 slider 调用 `App_Backlight_SetBrightness(value)`。
- App 层有自动息屏时间：
  - 默认 `10000U`
  - 设置页可设置 5s、10s、30s、Always On。
- 当前显示关闭是 `LCD_DisplayOff()`，不一定真正切断背光供电。
- 触摸或按键活动会刷新自动息屏计时。

### UI 页面

当前已有页面枚举包括：

- `APP_PAGE_WATCH`
- `APP_PAGE_MENU`
- `APP_PAGE_CONTROLCENTER`
- `APP_PAGE_WATCH_SELECT`
- `APP_PAGE_SETTING`
- `APP_PAGE_ABOUT`
- `APP_PAGE_SENSOR`
- `APP_PAGE_BACKLIGHT`
- `APP_PAGE_HEART`
- `APP_PAGE_ALTITUDE`
- `APP_PAGE_MOTION`
- `APP_PAGE_COMPASS`
- `APP_PAGE_WEATHER`
- `APP_PAGE_DATETIME`

已实现内容：

- 表盘页：彩色表盘、Simple Digital 表盘。
- 表盘选择页：切换表盘。
- 菜单页：Settings / Watch / About / Sensor。
- 控制中心：下滑呼出，上滑返回，亮度 slider，轻量帧动画。
- 设置页：Backlight Time、Date & Time。
- 背光时间页：5s / 10s / 30s / Always On。
- 传感器列表页：使用 flex 纵向滚动列表，滚动条已可配置隐藏。
- 传感器详情页：Motion / Heart / Altitude / Compass / Weather。
- 日期时间页：已有时间滚轮、日期滚轮、Save、SWAP。

## 7. 日期时间页当前状态

`app_ui.c` 里已有全局时间变量：

```c
static uint16_t hour = 12;
static uint16_t min = 38;
static uint16_t sec = 0;
static uint16_t year = 2026;
static uint16_t month = 7;
static uint16_t day = 11;
```

`watch_timer_cb()` 每秒推进这组时间变量，并刷新表盘 `time_label` / `date_label` / `day_arc`。

日期时间页已添加：

- `hour_rollor`
- `min_rollor`
- `sec_rollor`
- `year_rollor`
- `mon_rollor`
- `day_rollor`
- `datetime_mode`
- `datetime_hour_options`
- `datetime_min_sec_options`
- `datetime_year_options`：2020~2035
- `datetime_month_options`：01~12
- `datetime_day_options`：01~31

Save 逻辑：

- 时间模式保存 `hour/min/sec`。
- 日期模式读取 `year/month/day`。
- 如果日期不合法，例如 2 月 31 日，调用 `App_ShowToast("Invalid date")` 且不保存。
- 合法时保存并 Toast。

SWAP 逻辑：

- 在时间滚轮和日期滚轮之间切换显示。

注意：

- 当前年份选项范围是 `2020~2035`，后续可以扩展。
- 日期滚轮固定显示 `01~31`，保存时校验，不做动态联动。这是刻意选择，避免现阶段引入复杂联动。

## 8. 最近踩过的坑

- 不要直接 include LVGL 内部头文件，例如：

```c
#include "core/lv_obj_style_gen.h"
#include "widgets/roller/lv_roller.h"
#include "misc/lv_types.h"
```

应用层只保留：

```c
#include "lvgl.h"
```

否则容易出现“LVGL 源码报错”的假象。

- 页面跳转必须在 `App_HideAllPages()` 里隐藏所有可能盖在上层的页面。之前漏隐藏 `Page_Datetime`，导致右滑返回设置页时日期页还盖在上面，看起来像卡死。
- 自动 include 可能是 VS Code C/C++ 插件 Quick Fix 加的，不要盲信。
- `App_CreateButton()` 使用 `LV_ALIGN_CENTER`，所以按钮坐标原点是父对象中心，不是左上角。

## 9. 当前项目进度

已完成度粗略评估：

- LCD 底层：完成，约 90%。
- 触摸基础：完成，约 80%。
- LVGL 显示/输入移植：完成可用，约 80%。
- 页面框架/导航：完成主干，约 75%。
- 表盘：已有两个，约 70%。
- 控制中心：已有亮度功能和动画，约 70%。
- 设置页：背光时间和日期时间正在做，约 60%。
- 传感器 UI：页面有了，但真实传感器未接，约 35%。
- 真实传感器驱动：基本未开始，约 5%。
- FreeRTOS：未开始。
- RTC/掉电保持时间：未开始。

## 10. 推荐总路线图

### 阶段 1：当前 UI 功能闭环

目标：让手表像一个完整 UI 产品雏形。

- 完成 Date & Time 页面。
- 给 Save 后即时刷新表盘显示。
- 优化 Date & Time 页面布局和按钮文字。
- 整理 `app_ui.c` 过长问题，必要时拆出页面文件。
- 检查页面返回、控制中心、菜单跳转是否都有一致行为。

### 阶段 2：基础系统功能

目标：让项目从“UI Demo”变成“手表系统雏形”。

- 背光亮度与自动息屏稳定。
- KEY1/KEY2 角色确定：返回、亮屏、主页等。
- 时间源升级：
  - 短期：软件时间。
  - 中期：STM32 RTC。
  - 长期：掉电保持需要 RTC 电池或外部 RTC。
- 页面状态统一管理，减少全局变量混乱。

### 阶段 3：传感器接入

目标：让传感器页面显示真实数据。

推荐顺序：

1. I2C 扫描，确认设备地址。
2. 读 WHO_AM_I / ID 寄存器。
3. AHT21 温湿度，最简单，适合先做。
4. SPL06 气压/海拔。
5. MPU6050 姿态/步数。
6. LSM303 指南针。
7. EM7028 心率/血氧。

### 阶段 4：工程化和 RTOS

目标：提升就业相关能力。

- 拆分 UI 模块：`page_watch.c`、`page_setting.c`、`page_sensor.c` 等。
- 引入 FreeRTOS：
  - LVGL task。
  - Sensor task。
  - Input task。
  - Message queue / event group。
- 学习嵌入式工程常见结构：driver / service / app / ui。

### 阶段 5：作品集整理

目标：面向找工作展示。

- README 写清楚硬件、功能、架构、问题解决记录。
- 拍摄功能演示视频。
- 整理关键技术点：
  - SPI DMA LCD 刷屏。
  - LVGL 移植。
  - 触摸输入。
  - PWM 背光。
  - 传感器 I2C。
  - FreeRTOS 任务拆分。

## 11. 当前下一步建议

最推荐下一步：继续完成 Date & Time。

具体小任务：

1. Save 后立即刷新当前表盘 label，而不是等下一秒。
2. SWAP 按钮文字根据模式显示为 `Date` / `Time`，更直观。
3. 日期保存成功后同步刷新日期 label。
4. 测试非法日期 Toast。
5. 烧录真机测试返回、下滑控制中心、保存时间/日期。

不建议马上做：

- 不要马上上 FreeRTOS。
- 不要马上接所有传感器。
- 不要马上大拆文件，除非 `app_ui.c` 已经明显影响开发效率。

## 12. 给新 Codex 的提醒

- 当前上下文可能来自一个很长的旧窗口。新窗口请优先相信这个文档和实际源码。
- 用户会自己写一部分代码，请先检查用户代码，不要覆盖。
- 如果发现用户刚写的代码有拼写/漏分号/复制错对象，属于简单错误，可以直接修。
- 如果涉及设计选择，比如“新页面还是同页切换”，先解释方案，再等用户确认。
- 不要改 LVGL 源码。
- 不要删除用户写的功能，除非明确要求。
- 每次编译后报告真实输出，不要说“应该能过”。

