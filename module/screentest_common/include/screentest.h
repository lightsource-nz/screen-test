#ifndef _SCREENTEST_H
#define _SCREENTEST_H

#include <light.h>
#include <light_display.h>

#include <stdint.h>

// TODO implement version fields properly
#define ST_VERSION_STR                  "0.1.0"

#define ST_INFO_STR                     "screen-test v" CF_VERSION_STR

// TODO make display count configurable at runtime
#define ST_DISPLAY_COUNT                2

// default render context geometry -- matches this file's own hardcoded values before
// they became overridable, i.e. the small portrait OLED test rigs (screentest_po13,
// which has no screentest.h of its own and falls back to this file). apps with a
// differently-shaped display (e.g. screentest_ws_touch169's 240x280 16bpp panel)
// provide their own complete screentest.h that shadows this one entirely, including
// these four -- see light_display_ioport's PORT_SPI_1 wiring convention for why that
// shadowing already has to work this way for ST_DISPLAY_COUNT et al
#define ST_RENDER_WIDTH                 64
#define ST_RENDER_HEIGHT                128
#define ST_RENDER_BPP                   1
#define ST_RENDER_ROTATION              REND_ROTATE_90
// roughly centered in the 128x64 logical (post-rotation) canvas the animated test circle
// draws against -- also overridable per-app, same rationale as the geometry above
#define ST_RENDER_CIRCLE_X              64
#define ST_RENDER_CIRCLE_Y              32

#define ST_DISPLAY_1_PIN_CS             17
#define ST_DISPLAY_1_PIN_DC             16
#define ST_DISPLAY_1_PIN_SCK            18
#define ST_DISPLAY_1_PIN_TX             19
#define ST_DISPLAY_1_PIN_RESET          20

extern struct display_device *_display[ST_DISPLAY_COUNT];

extern void __screentest_hardware_init();

#endif