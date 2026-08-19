#ifndef _SCREENTEST_CONFIG_H
#define _SCREENTEST_CONFIG_H

// board pinout and device construction (ST_DISPLAY_*, ST_TOUCH_*, ST_IMU_*)
#include <screentest_hw_ws_touch169.h>

// ONLY what this board does differently from screentest_common's defaults. everything not
// named here -- frame rate, circle animation, swipe and tilt tuning -- comes from there, so
// changing a shared value is one edit rather than one per app

#define ST_DISPLAY_COUNT                1

// this panel's real geometry rather than the small OLED rigs' 64x128 1bpp. no rotation: the
// ST7789 driver already declares width/height in the panel's own native orientation, so the
// logical and physical spaces coincide at rest -- which is also what lets touch coordinates
// reach light_ui without a board-specific correction
#define ST_RENDER_WIDTH                 ST_DISPLAY_WIDTH
#define ST_RENDER_HEIGHT                ST_DISPLAY_HEIGHT
#define ST_RENDER_BPP                   16
#define ST_RENDER_ROTATION              LIGHT_DRAW_ROTATE_0

// centred on this panel's much larger canvas. the shared default centres on the OLED
// geometry, which is nowhere near the middle here
#define ST_RENDER_CIRCLE_X              (ST_RENDER_WIDTH / 2)
#define ST_RENDER_CIRCLE_Y              (ST_RENDER_HEIGHT / 2)

#endif
