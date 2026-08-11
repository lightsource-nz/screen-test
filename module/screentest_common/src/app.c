#include <screentest.h>
#include <light_canvas.h>
#include <light_platform.h>
#include <module/mod_light_canvas.h>
#include <module/mod_light_display.h>
#include <module/mod_light_backlight.h>
#include <module/mod_light_imu.h>
#include <module/mod_light_touch.h>

#include "screentest_internal.h"

static struct rend_context *render;
static struct canvas_context *canvas;
struct display_device *_display[ST_DISPLAY_COUNT];
struct touch_device *_touch_main;
struct imu_device *_imu_main;

// where the animated circle currently sits, and the slide it's partway through. each swipe
// starts a fresh slide of ST_SWIPE_MOVE_DISTANCE in the swiped direction, from wherever the
// circle happens to be -- so swiping again mid-slide redirects it rather than queueing.
// _touch_main is NULL on boards with no touch hardware, where the circle simply stays put
static int32_t circle_x = ST_RENDER_CIRCLE_X;
static int32_t circle_y = ST_RENDER_CIRCLE_Y;
static int32_t slide_from_x, slide_from_y;
static int32_t slide_delta_x, slide_delta_y;
static uint32_t slide_start_ms;
static bool slide_active;

// the box the circle occupies, at its MAXIMUM radius rather than its current one, so a
// shrinking circle still invalidates its own previous extent. signed, because a circle near
// an edge extends past it and rend_point2d's uint16_t would wrap instead of clipping --
// light_canvas takes signed regions for exactly this reason and clips them itself.
//
// only where the circle is NOW: light_canvas re-invalidates whatever the previous frame
// pushed, which is where the circle just was, so the area it vacated is covered without
// this having to track it
static struct canvas_region _circle_region(int32_t cx, int32_t cy)
{
        return (struct canvas_region) {
                (int16_t)(cx - ST_CIRCLE_MAX_RADIUS), (int16_t)(cy - ST_CIRCLE_MAX_RADIUS),
                (int16_t)(cx + ST_CIRCLE_MAX_RADIUS), (int16_t)(cy + ST_CIRCLE_MAX_RADIUS)
        };
}

static void screentest_event(const struct light_module *module, uint8_t event, void *arg);
static uint8_t screentest_main(struct light_application *app);

void __screentest_hardware_init();

Light_Application_Define(screentest, screentest_event, screentest_main,
                                &rend,
                                &light_canvas,
                                &light_display,
                                &light_imu,
                                &light_backlight,
                                &light_touch,
                                &light_core);

static uint32_t last_run;

// radius of the animated test circle at a given moment: grows from 1px to
// ST_CIRCLE_MAX_RADIUS at ST_CIRCLE_GROWTH_PX_PER_S, then restarts.
//
// derived from the clock rather than advanced once per frame, so the animation keeps the
// same real-world speed if the frame rate changes, and doesn't slow down when a frame is
// skipped because a display is still flushing the last one. it also can't drift, since
// each frame recomputes from absolute time rather than accumulating
// keeps a coordinate on the canvas by wrapping rather than clamping, so the circle driven
// off one edge reappears on the opposite one. C's % keeps the sign of the dividend, hence
// the extra fold for negatives
static int32_t _wrap(int32_t v, int32_t limit)
{
        v %= limit;
        return v < 0 ? v + limit : v;
}
// begins a slide of ST_SWIPE_MOVE_DISTANCE in the swiped direction, starting from wherever
// the circle currently is. y grows downward, so SWIPE_UP is negative dy
static void _start_slide(uint8_t gesture_type, uint32_t now)
{
        slide_from_x = circle_x;
        slide_from_y = circle_y;
        slide_delta_x = 0;
        slide_delta_y = 0;
        switch(gesture_type) {
        case TOUCH_GESTURE_SWIPE_UP:    slide_delta_y = -ST_SWIPE_MOVE_DISTANCE; break;
        case TOUCH_GESTURE_SWIPE_DOWN:  slide_delta_y =  ST_SWIPE_MOVE_DISTANCE; break;
        case TOUCH_GESTURE_SWIPE_LEFT:  slide_delta_x = -ST_SWIPE_MOVE_DISTANCE; break;
        case TOUCH_GESTURE_SWIPE_RIGHT: slide_delta_x =  ST_SWIPE_MOVE_DISTANCE; break;
        default:
                return;
        }
        slide_start_ms = now;
        slide_active = true;
}
// advances an in-flight slide. driven off the clock for the same reasons as the radius
// below: independent of frame rate, and immune to a skipped frame
static void _advance_slide(uint32_t now)
{
        if(!slide_active)
                return;

        uint32_t elapsed = now - slide_start_ms;
        if(elapsed >= ST_SWIPE_MOVE_MS) {
                circle_x = _wrap(slide_from_x + slide_delta_x, render->dim_x);
                circle_y = _wrap(slide_from_y + slide_delta_y, render->dim_y);
                slide_active = false;
                return;
        }
        circle_x = _wrap(slide_from_x + (int32_t)(slide_delta_x * (int32_t)elapsed) / ST_SWIPE_MOVE_MS,
                        render->dim_x);
        circle_y = _wrap(slide_from_y + (int32_t)(slide_delta_y * (int32_t)elapsed) / ST_SWIPE_MOVE_MS,
                        render->dim_y);
}
// speed along one axis for a given tilt, in pixels per second. ramps from zero at the edge
// of the deadzone rather than jumping straight to a proportional speed, so the circle eases
// into motion instead of snapping the moment the threshold is crossed
static int32_t _tilt_velocity(int32_t accel_mg)
{
        int32_t magnitude = accel_mg < 0 ? -accel_mg : accel_mg;
        if(magnitude <= ST_TILT_DEADZONE_MG)
                return 0;

        // 1000mg is a full 90 degree tilt of that axis into gravity, so that is what counts
        // as "full deflection" -- clamped, since a shake can read well past 1g
        int32_t over = magnitude - ST_TILT_DEADZONE_MG;
        const int32_t span = 1000 - ST_TILT_DEADZONE_MG;
        if(over > span)
                over = span;

        int32_t speed = (over * ST_TILT_MAX_SPEED_PX_PER_S) / span;
        return accel_mg < 0 ? -speed : speed;
}
// steers the circle by tilting the board. driven off the clock for the same reason as the
// slide above -- constant speed whatever the frame rate.
//
// suppressed while a slide is running: a slide sets an ABSOLUTE position from where it
// began, so anything tilt contributed mid-slide would be overwritten on the next tick
// anyway. swipe to fling, tilt to steer
static uint32_t tilt_last_ms;
// leftover sub-pixel motion, in milli-pixels. without this the whole feature stalls at low
// tilt: at 30px/s a 10ms tick is 0.3px, which integer-divides to zero every single time, so
// the circle would never move at all below a third of full speed
static int32_t tilt_residue_x, tilt_residue_y;
static void _advance_tilt(uint32_t now)
{
        uint32_t elapsed = now - tilt_last_ms;
        tilt_last_ms = now;

        if(!_imu_main || slide_active)
                return;
        // the first call after boot, or the tick after a long stall -- charging the whole
        // gap as tilt time would fling the circle across the canvas in one step
        if(!elapsed || elapsed > ST_TILT_MAX_STEP_MS)
                return;

        int32_t vx = ST_IMU_TILT_X_SIGN * _tilt_velocity(_imu_main->accel_mg[ST_IMU_TILT_X_AXIS]);
        int32_t vy = ST_IMU_TILT_Y_SIGN * _tilt_velocity(_imu_main->accel_mg[ST_IMU_TILT_Y_AXIS]);

        tilt_residue_x += vx * (int32_t)elapsed;
        tilt_residue_y += vy * (int32_t)elapsed;
        int32_t step_x = tilt_residue_x / 1000;
        int32_t step_y = tilt_residue_y / 1000;
        tilt_residue_x -= step_x * 1000;
        tilt_residue_y -= step_y * 1000;

        if(step_x)
                circle_x = _wrap(circle_x + step_x, render->dim_x);
        if(step_y)
                circle_y = _wrap(circle_y + step_y, render->dim_y);
}
static uint16_t _circle_radius(uint32_t now)
{
        const uint32_t cycle_ms =
                        ((uint32_t)ST_CIRCLE_MAX_RADIUS * 1000) / ST_CIRCLE_GROWTH_PX_PER_S;
        uint32_t phase = now % cycle_ms;
        return 1 + (uint16_t)((phase * ST_CIRCLE_MAX_RADIUS) / cycle_ms);
}

void main(int argc, char **argv)
{
        light_framework_init();
        light_framework_run(argc, argv);

}
  
static void screentest_event(const struct light_module *module, uint8_t event, void *arg)
{
        switch(event) {
        case LF_EVENT_MODULE_LOAD:;
                // geometry is overridable per-app via ST_RENDER_WIDTH/HEIGHT/BPP/ROTATION in
                // screentest.h (see this file's own default, used as a fallback by apps with
                // no screentest.h of their own, e.g. screentest_po13). the OLED default here
                // creates the render context at the panels' actual physical 64x128 portrait
                // dimensions and rotates it, rather than creating it pre-rotated (128x64) with
                // no rotation transform, which would mismatch the real device buffers and
                // scramble pixel positions (matches crossfire's own working setup for the same
                // panel -- see crossfire.c's crossfire_display_init())
                render = rend_context_create(
                        "screentest_render_main", ST_RENDER_WIDTH, ST_RENDER_HEIGHT, ST_RENDER_BPP);
                rend_context_set_rotation(render, ST_RENDER_ROTATION);
                render->point_radius = 2;
                light_debug("passing control to display hardware setup function","");
                __screentest_hardware_init();
                for(uint8_t i = 0; i < ST_DISPLAY_COUNT; i++) {
                        light_display_set_render_context(_display[i], render);
                }
                // frame pacing, double buffering and region flushing all live here now --
                // the canvas has to be created after the displays exist, since it presents
                // onto them
                canvas = light_canvas_create(render, _display, ST_DISPLAY_COUNT);
                light_canvas_enable_double_buffer(canvas);
                light_canvas_set_frame_rate(canvas, ST_FRAME_RATE);
                light_info("display pipeline setup complete");
        break;
        // TODO implement unregister for event hooks
        case LF_EVENT_MODULE_UNLOAD:
        break; 
        }
}
static uint8_t screentest_main(struct light_application *app)
{
        uint32_t now = light_platform_get_time_since_init();
        light_trace("enter Screentest application task, time=%dms, time since last run=%dms", now, last_run - now);

        // gestures are recognised on release by light_touch itself, so this only has to
        // collect them. checked every tick rather than once per frame: light_touch's own
        // periodic task runs independently of this app's frame rate, and only one gesture
        // is held at a time, so collecting at frame rate could drop one
        struct touch_gesture gesture;
        if(_touch_main && light_touch_take_gesture(_touch_main, &gesture)) {
                static const char *const gesture_name[] = {
                        "none", "swipe up", "swipe down", "swipe left", "swipe right"
                };
                light_info("gesture: %s from (%d,%d) to (%d,%d) [%s]",
                                gesture_name[gesture.type],
                                gesture.start_x, gesture.start_y,
                                gesture.end_x, gesture.end_y,
                                gesture.from_hardware ? "hardware" : "software");
                _start_slide(gesture.type, now);
        }

        // orientation changes are collected the same way gestures are, and for the same
        // reason: light_imu holds one at a time, so checking every tick rather than once per
        // frame keeps a change from being dropped
        uint8_t orientation;
        if(_imu_main && light_imu_take_orientation(_imu_main, &orientation)) {
                static const char *const orientation_name[] = {
                        "unknown", "portrait", "portrait-flipped",
                        "landscape-left", "landscape-right", "face-up", "face-down"
                };
                // the temperature is logged alongside because nothing else surfaces it, and
                // a plainly wrong value (rather than a plausible room-temperature one) is
                // the quickest signal that its scaling constant is off
                light_info("orientation: %s (accel %d,%d,%d mg, die %d mC)",
                                orientation_name[orientation],
                                _imu_main->accel_mg[IMU_AXIS_X],
                                _imu_main->accel_mg[IMU_AXIS_Y],
                                _imu_main->accel_mg[IMU_AXIS_Z],
                                _imu_main->temperature_mc);
        }

        _advance_slide(now);
        _advance_tilt(now);

        // the canvas owns the frame deadline, the buffer swap and the check for a display
        // still reading the buffer -- on false there is simply nothing to do this tick
        if(light_canvas_frame_begin(canvas)) {
                rend_draw_circle(render, (rend_point2d) {(uint16_t)circle_x, (uint16_t)circle_y},
                                _circle_radius(now), true);
                // the whole buffer was cleared and redrawn, but the only pixels that can
                // differ from what the panel already shows are the circle's, so only a box
                // covering it is invalidated. where it was last frame is covered by the
                // canvas carrying the previous frame's regions forward
                light_canvas_invalidate_rect(canvas, _circle_region(circle_x, circle_y));
                light_canvas_frame_end(canvas);
        }

        last_run = now;
        return LF_STATUS_RUN;
}