cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME OV_Watch)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

set(LVGL_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/Lvgl)
file(GLOB LVGL_ROOT_SOURCES CONFIGURE_DEPENDS
    ${LVGL_ROOT}/src/*.c
)
file(GLOB_RECURSE LVGL_SOURCES CONFIGURE_DEPENDS
    ${LVGL_ROOT}/src/core/*.c
    ${LVGL_ROOT}/src/display/*.c
    ${LVGL_ROOT}/src/draw/*.c
    ${LVGL_ROOT}/src/draw/convert/*.c
    ${LVGL_ROOT}/src/draw/sw/*.c
    ${LVGL_ROOT}/src/draw/sw/blend/*.c
    ${LVGL_ROOT}/src/drivers/display/lcd/*.c
    ${LVGL_ROOT}/src/drivers/display/st7789/*.c
    ${LVGL_ROOT}/src/font/lv_font.c
    ${LVGL_ROOT}/src/font/fmt_txt/*.c
    ${LVGL_ROOT}/src/font/lv_font_montserrat_14.c
    ${LVGL_ROOT}/src/font/lv_font_montserrat_16.c
    ${LVGL_ROOT}/src/font/lv_font_montserrat_18.c
    ${LVGL_ROOT}/src/font/lv_font_montserrat_44.c
    ${LVGL_ROOT}/src/indev/*.c
    ${LVGL_ROOT}/src/libs/bin_decoder/*.c
    ${LVGL_ROOT}/src/layouts/*.c
    ${LVGL_ROOT}/src/layouts/flex/*.c
    ${LVGL_ROOT}/src/layouts/grid/*.c
    ${LVGL_ROOT}/src/misc/*.c
    ${LVGL_ROOT}/src/misc/cache/*.c
    ${LVGL_ROOT}/src/misc/cache/class/*.c
    ${LVGL_ROOT}/src/misc/cache/instance/*.c
    ${LVGL_ROOT}/src/osal/lv_os.c
    ${LVGL_ROOT}/src/osal/lv_os_none.c
    ${LVGL_ROOT}/src/stdlib/lv_mem.c
    ${LVGL_ROOT}/src/stdlib/builtin/*.c
    ${LVGL_ROOT}/src/themes/*.c
    ${LVGL_ROOT}/src/themes/default/*.c
    ${LVGL_ROOT}/src/tick/*.c
    ${LVGL_ROOT}/src/widgets/*/*.c
)
list(APPEND LVGL_SOURCES ${LVGL_ROOT_SOURCES})
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/draw/(dma2d|espressif|eve|nanovg|nema_gfx|nxp|opengles|renesas|sdl|vg_lite)/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/draw/convert/(helium|neon|riscv_v)/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/draw/sw/blend/(helium|neon|riscv_v)/.*")
list(FILTER LVGL_SOURCES EXCLUDE REGEX ".*/misc/lv_profiler_builtin_posix\\.c$")

add_library(lvgl STATIC ${LVGL_SOURCES})
target_include_directories(lvgl PUBLIC
    ${LVGL_ROOT}
    ${LVGL_ROOT}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
)
target_compile_definitions(lvgl PUBLIC
    LV_CONF_INCLUDE_SIMPLE
    LV_KCONFIG_IGNORE
)
target_compile_options(lvgl PRIVATE -Os)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    Core/Src/app.c
    Core/Src/app_backlight.c
    Core/Src/app_ui.c
    Core/Src/app_sensor.c
    Core/Src/app_anim_frames.c
    Core/Src/key.c
    Core/Src/lcd.c
    Core/Src/touch.c
    Lvgl/porting/lv_port_lcd_stm32.c
    Lvgl/porting/lv_port_indev.c
)
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Os)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${LVGL_ROOT}
    ${LVGL_ROOT}/src
    ${LVGL_ROOT}/porting
    Core/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
    lvgl

    # Add user defined libraries
)
