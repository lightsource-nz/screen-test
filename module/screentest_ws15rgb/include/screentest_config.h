#ifndef _SCREENTEST_CONFIG_H
#define _SCREENTEST_CONFIG_H

// rig wiring and device construction (ST_DISPLAY_*)
#include <light_ui_hw_ws15rgb.h>

// ONLY what this rig does differently from screentest_common's defaults. everything not
// named here -- frame rate, swipe and tilt tuning -- comes from there, so changing a shared
// value is one edit rather than one per app

// one panel. the Pico-OLED-1.3 on the same host is a second display this app deliberately
// does not drive: the point of this binary is the new panel on its own, so a blank frame is
// attributable to it rather than to whichever of two devices went wrong
#define ST_DISPLAY_COUNT                1

// 128x128 at 16bpp, against the shared defaults' 64x128 1bpp. no rotation: the glass is
// square, so there is no native orientation to correct for and the logical and physical
// spaces coincide
#define ST_RENDER_WIDTH                 ST_DISPLAY_WIDTH
#define ST_RENDER_HEIGHT                ST_DISPLAY_HEIGHT
#define ST_RENDER_BPP                   16
#define ST_RENDER_ROTATION              LIGHT_DRAW_ROTATE_0

// centred on this panel's canvas. the shared defaults centre on the 64x128 OLED geometry,
// which is nowhere near the middle here
#define ST_RENDER_CIRCLE_X              (ST_RENDER_WIDTH / 2)
#define ST_RENDER_CIRCLE_Y              (ST_RENDER_HEIGHT / 2)

//   the circle grows until it touches all four edges, rather than stopping at the shared
// 16px. that is the point of it on a bring-up: a circle that reaches every edge of the glass
// and comes back is a check on the window addressing across the whole panel, and any row or
// column the driver fails to reach shows up as a flat side. 64 = half of 128
#define ST_CIRCLE_MAX_RADIUS            64

#endif
