#ifndef _SCREENTEST_H
#define _SCREENTEST_H

#include <light.h>
#include <light_platform.h>
#include <light_display.h>
#include <light_display_sh1106.h>

#include <rend.h>

#include <stdint.h>

// TODO implement version fields properly
#define ST_VERSION_STR                  "0.1.0"

// ST_VERSION_STR, not CF_VERSION_STR -- the latter is crossfire's macro and is not defined
// anywhere in this project. Copied across with the rest of this header, and it compiled only
// because nothing ever expands ST_INFO_STR; the first use would have been a build failure
#define ST_INFO_STR                     "screen-test v" ST_VERSION_STR

// TODO make display count configurable at runtime
#define ST_DISPLAY_COUNT                1

// this panel is physically 64 wide x 128 tall, portrait -- see screentest_common's
// screentest.h for why the render context is created at those dimensions and rotated
// rather than created pre-rotated
#define ST_RENDER_WIDTH                 64
#define ST_RENDER_HEIGHT                128
#define ST_RENDER_BPP                   1
#define ST_RENDER_ROTATION              REND_ROTATE_90
#define ST_RENDER_CIRCLE_X              64
#define ST_RENDER_CIRCLE_Y              32

#define ST_DISPLAY_0_PORT_ID            PORT_SPI_0
#define ST_DISPLAY_0_PIN_CS             17
#define ST_DISPLAY_0_PIN_DC             16
#define ST_DISPLAY_0_PIN_SCK            18
#define ST_DISPLAY_0_PIN_TX             19
#define ST_DISPLAY_0_PIN_RESET          20

#endif