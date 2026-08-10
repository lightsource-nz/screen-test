#ifndef _SCREENTEST_H
#define _SCREENTEST_H

#include <light.h>
#include <light_display.h>
#include <light_touch.h>
// board pinout and device construction (ST_DISPLAY_*, ST_TOUCH_*) -- everything below is
// application-level tuning of the demo that runs on top of it
#include <screentest_hw_ws_touch169.h>

#include <stdint.h>

// TODO implement version fields properly
#define ST_VERSION_STR                  "0.1.0"

#define ST_INFO_STR                     "screen-test v" CF_VERSION_STR

#define ST_DISPLAY_COUNT                1

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
#define ST_SWIPE_MOVE_DISTANCE          40
#define ST_SWIPE_MOVE_MS                400

// tilt steering -- see screentest_common's screentest.h for what these control. the
// ST_IMU_TILT_* axis mapping is NOT repeated here: it comes from
// screentest_hw_ws_touch169.h, included above, because it describes how the chip is mounted
// on this board rather than how the demo behaves
#define ST_TILT_DEADZONE_MG             120
#define ST_TILT_MAX_SPEED_PX_PER_S      120
#define ST_TILT_MAX_STEP_MS             250

extern struct display_device *_display[ST_DISPLAY_COUNT];
extern struct touch_device *_touch_main;
extern struct imu_device *_imu_main;

extern void __screentest_hardware_init();

#endif
