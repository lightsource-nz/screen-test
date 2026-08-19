#ifndef _SCREENTEST_H
#define _SCREENTEST_H

// every app that uses this shared demo supplies a screentest_config.h naming ONLY what its
// board does differently. everything below is a default, applied where the config was
// silent.
//
// this used to work the other way round: an app shadowed this whole header by putting its
// own screentest.h earlier on the include path. that made adding any shared setting a
// two-place edit with no way to notice the second one had been missed except a build
// failure, and it depended entirely on the app's include directory happening to be listed
// before this module's -- which nothing enforced
#include <screentest_config.h>

#include <light.h>
#include <light_display.h>
#include <light_backlight.h>
#include <light_imu.h>
#include <light_touch.h>

#include <stdint.h>

// TODO implement version fields properly
#ifndef ST_VERSION_STR
#define ST_VERSION_STR                  "0.1.0"
#endif

// how many displays __screentest_hardware_init() is expected to populate
#ifndef ST_DISPLAY_COUNT
#define ST_DISPLAY_COUNT                2
#endif

// render context geometry. the defaults are the small portrait OLED rigs' -- the render
// context is created at the panel's real physical dimensions and rotated, rather than being
// created pre-rotated with no transform, which would mismatch the device buffers and
// scramble pixel positions
#ifndef ST_RENDER_WIDTH
#define ST_RENDER_WIDTH                 64
#endif
#ifndef ST_RENDER_HEIGHT
#define ST_RENDER_HEIGHT                128
#endif
#ifndef ST_RENDER_BPP
#define ST_RENDER_BPP                   1
#endif
#ifndef ST_RENDER_ROTATION
#define ST_RENDER_ROTATION              LIGHT_DRAW_ROTATE_90
#endif

// where the animated circle starts, centred in the logical (post-rotation) canvas
#ifndef ST_RENDER_CIRCLE_X
#define ST_RENDER_CIRCLE_X              (ST_RENDER_HEIGHT / 2)
#endif
#ifndef ST_RENDER_CIRCLE_Y
#define ST_RENDER_CIRCLE_Y              (ST_RENDER_WIDTH / 2)
#endif

// how often the shared app redraws. a display that can't flush a frame this often skips
// frames rather than falling behind, since light_canvas gates on the transfer still running
#ifndef ST_FRAME_RATE
#define ST_FRAME_RATE                   24
#endif
// the animated test circle grows from 1px to this radius, then restarts...
#ifndef ST_CIRCLE_MAX_RADIUS
#define ST_CIRCLE_MAX_RADIUS            16
#endif
// ...at this many pixels per second. expressed as a RATE rather than a per-frame step so the
// animation runs at the same speed whatever ST_FRAME_RATE is, and doesn't lurch when a frame
// gets skipped
#ifndef ST_CIRCLE_GROWTH_PX_PER_S
#define ST_CIRCLE_GROWTH_PX_PER_S       8
#endif
// each swipe slides the circle this far in the swiped direction, over this long. the
// circle's centre wraps at the canvas edges, so it can be driven around indefinitely
#ifndef ST_SWIPE_MOVE_DISTANCE
#define ST_SWIPE_MOVE_DISTANCE          40
#endif
#ifndef ST_SWIPE_MOVE_MS
#define ST_SWIPE_MOVE_MS                400
#endif

// tilt steering: how far the board must be tilted before the circle starts moving, and how
// fast it travels at full tilt. the deadzone matters more than it looks -- a board lying
// flat still reads tens of mg of noise on the horizontal axes, and without it the circle
// creeps continuously
#ifndef ST_TILT_DEADZONE_MG
#define ST_TILT_DEADZONE_MG             120
#endif
#ifndef ST_TILT_MAX_SPEED_PX_PER_S
#define ST_TILT_MAX_SPEED_PX_PER_S      120
#endif
// a gap longer than this (a stall, or the first tick after boot) is not treated as elapsed
// tilt time -- otherwise the circle lurches across the canvas in one step
#ifndef ST_TILT_MAX_STEP_MS
#define ST_TILT_MAX_STEP_MS             250
#endif

// which IMU axis steers which canvas axis. NOT board-specific: light_imu already rotates
// every sample into the device frame using the board's own axis map, so by the time the demo
// sees it, +X is right across the display and +Y is up it whatever the mounting.
//
// what remains is the relationship between that frame and the canvas, which is fixed. a ball
// rolls toward whichever edge is lowered, i.e. AGAINST the accelerometer reading on that
// axis -- hence the negation on X. Y escapes it only because canvas Y grows DOWNWARD while
// device Y points up, and the two negations cancel
#ifndef ST_IMU_TILT_X_AXIS
#define ST_IMU_TILT_X_AXIS              IMU_AXIS_X
#define ST_IMU_TILT_X_SIGN              (-1)
#define ST_IMU_TILT_Y_AXIS              IMU_AXIS_Y
#define ST_IMU_TILT_Y_SIGN              1
#endif

extern struct display_device *_display[ST_DISPLAY_COUNT];
// NULL on boards with no touch hardware -- __screentest_hardware_init() only assigns this on
// a board that has one; everywhere else it stays zero-initialised and the touch-triggered
// animation is simply skipped
extern struct touch_device *_touch_main;
// NULL on boards with no IMU, on exactly the same terms as _touch_main above
extern struct imu_device *_imu_main;

extern void __screentest_hardware_init();

#endif
