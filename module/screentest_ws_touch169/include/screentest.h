#ifndef _SCREENTEST_H
#define _SCREENTEST_H

#include <light.h>
#include <light_display.h>
#include <light_touch.h>

#include <stdint.h>

// TODO implement version fields properly
#define ST_VERSION_STR                  "0.1.0"

#define ST_INFO_STR                     "screen-test v" CF_VERSION_STR

#define ST_DISPLAY_COUNT                1

// confirmed from the board's schematic + wiki pin table during bring-up
#define ST_DISPLAY_PIN_DC               8
#define ST_DISPLAY_PIN_CS               9
#define ST_DISPLAY_PIN_SCK              10
#define ST_DISPLAY_PIN_MOSI             11
#define ST_DISPLAY_PIN_RESET            13
#define ST_DISPLAY_PIN_BL               25

// CST816T touch controller -- shared I2C1 bus (also used by IMU/RTC, not yet
// implemented), confirmed from the board's schematic + wiki pin table during bring-up
#define ST_TOUCH_PIN_SDA                6
#define ST_TOUCH_PIN_SCL                7
#define ST_TOUCH_PIN_INT                21
#define ST_TOUCH_PIN_RST                22
#define ST_TOUCH_X_MAX                  ST_DISPLAY_WIDTH
#define ST_TOUCH_Y_MAX                  ST_DISPLAY_HEIGHT

// tried width/height swapped (280x240) as an experiment to explain the noise strip --
// disproved, not confirmed: it made the noise strip wider and fragmented the test circle
// into a horizontally-repeating row of smaller circles (a column-address-wraparound
// signature -- 280 exceeds this panel's real native column capacity). back to the
// product spec's 240x280, which the evidence now says was correct all along
#define ST_DISPLAY_WIDTH                240
#define ST_DISPLAY_HEIGHT               280

// screentest_common's shared app.c creates its render context at these dimensions (see
// its own screentest.h for the default, sized for the small portrait OLED test rigs --
// this panel needs its own, real geometry instead of that fallback). no rotation for a
// first pass; the OLED apps need REND_ROTATE_90 because their physical panel is portrait
// but this driver already declares width/height matching the panel's own native
// orientation, so no transform should be needed unless on-hardware testing says otherwise
#define ST_RENDER_WIDTH                 ST_DISPLAY_WIDTH
#define ST_RENDER_HEIGHT                ST_DISPLAY_HEIGHT
#define ST_RENDER_BPP                   16
#define ST_RENDER_ROTATION              REND_ROTATE_0
// centered in whatever ST_RENDER_WIDTH/HEIGHT currently are, rather than the OLED rigs'
// hardcoded (64, 32) -- see screentest_common's screentest.h for that default, which
// isn't remotely centered on this panel's much larger canvas
#define ST_RENDER_CIRCLE_X              (ST_RENDER_WIDTH / 2)
#define ST_RENDER_CIRCLE_Y              (ST_RENDER_HEIGHT / 2)

// see screentest_common's screentest.h for what these control -- this header shadows that
// one entirely, so they have to be repeated here rather than inherited
#define ST_FRAME_RATE                   24
#define ST_CIRCLE_MAX_RADIUS            16
#define ST_CIRCLE_GROWTH_PX_PER_S       8

extern struct display_device *_display[ST_DISPLAY_COUNT];
extern struct touch_device *_touch_main;

extern void __screentest_hardware_init();

#endif
