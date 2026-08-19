#ifndef _SCREENTEST_H
#define _SCREENTEST_H

#include <light.h>
#include <light_platform.h>
#include <light_display.h>
#include <light_display_sh1106.h>

#include <light_draw.h>

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
#define ST_RENDER_ROTATION              LIGHT_DRAW_ROTATE_90
#define ST_RENDER_CIRCLE_X              64
#define ST_RENDER_CIRCLE_Y              32

#define ST_DISPLAY_0_PORT_ID            PORT_I2C_0
#define ST_DISPLAY_0_PIN_SCL            5
#define ST_DISPLAY_0_PIN_SDA            4
// this board breaks out only SCL/SDA/power -- no hardware RESET line
#define ST_DISPLAY_0_PIN_RESET          LIGHT_IOPORT_PIN_NONE
// common default 7-bit I2C address for this controller family (SA0/D-C# pin tied low) --
// change to 0x3D if the module's address-select pin is tied high instead
#define ST_DISPLAY_0_I2C_ADDR           0x3C

#endif
