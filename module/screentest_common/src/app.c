#include <screentest.h>
#include <light_platform.h>
#include <module/mod_light_display.h>
#include <module/mod_light_touch.h>

#include "screentest_internal.h"

static struct rend_context *render;
struct display_device *_display[ST_DISPLAY_COUNT];
struct touch_device *_touch_main;

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

// bounding box of everything drawn into the frame currently on the panel. each update has
// to cover this as well as its own content: whatever was drawn last frame and ISN'T drawn
// again this frame still has to be repainted, or it stays on the panel forever. the buffer
// is fully cleared and redrawn every frame, so it is only ever the panel that goes stale.
//
// tracking the real previous box matters precisely because the circle MOVES: covering only
// where it is now would strand where it just was
static int32_t panel_bx0, panel_by0, panel_bx1, panel_by1;
static bool panel_box_valid;

// grows an accumulating bounding box to cover a circle. signed, because a circle near an
// edge extends past it and rend_point2d's uint16_t would wrap instead of clipping
static void _bbox_add_circle(int32_t cx, int32_t cy, int32_t r,
                        int32_t *x0, int32_t *y0, int32_t *x1, int32_t *y1)
{
        if(cx - r < *x0) *x0 = cx - r;
        if(cy - r < *y0) *y0 = cy - r;
        if(cx + r > *x1) *x1 = cx + r;
        if(cy + r > *y1) *y1 = cy + r;
}

static void screentest_event(const struct light_module *module, uint8_t event, void *arg);
static uint8_t screentest_main(struct light_application *app);
static void screentest_set_frame_rate(uint32_t frame_rate);

void __screentest_hardware_init();

Light_Application_Define(screentest, screentest_event, screentest_main,
                                &rend,
                                &light_display,
                                &light_touch,
                                &light_core);

static uint32_t last_run;
static uint32_t next_frame;
static uint32_t frame_interval_ms;
static uint32_t frame_counter;

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
                rend_context_enable_double_buffer(render);
                render->point_radius = 2;
                frame_counter = 0;
                screentest_set_frame_rate(ST_FRAME_RATE);
                light_debug("passing control to display hardware setup function","");
                __screentest_hardware_init();
                for(uint8_t i = 0; i < ST_DISPLAY_COUNT; i++) {
                        light_display_set_render_context(_display[i], render);
                }
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
        _advance_slide(now);

        // the buffer we're about to swap into is the one that was in flight two frames
        // ago -- if some device is still flushing from it, wait rather than start
        // drawing over data a DMA transfer is still reading (see
        // rend_context_swap_buffers()/light_display_render_context_busy())
        if(now >= next_frame && !light_display_render_context_busy(render)) {
                next_frame += frame_interval_ms;
                frame_counter++;
                rend_context_swap_buffers(render);
                rend_draw_clear(render);
//              rend_draw_point(display->render_ctx, (rend_point2d) {ST_RENDER_CIRCLE_X, ST_RENDER_CIRCLE_Y});
                rend_draw_circle(render, (rend_point2d) {(uint16_t)circle_x, (uint16_t)circle_y},
                                _circle_radius(now), true);
//              rend_debug_buffer_print_stdout(display->render_ctx);

                // the whole buffer was cleared and redrawn above, but the only pixels that
                // can differ from what the panel already shows are the circle's -- so only
                // a box covering it is pushed. its MAX radius is used rather than its
                // current one, so a shrinking circle still erases its own previous extent
                int32_t cx0 = INT32_MAX, cy0 = INT32_MAX, cx1 = INT32_MIN, cy1 = INT32_MIN;
                _bbox_add_circle(circle_x, circle_y, ST_CIRCLE_MAX_RADIUS,
                                &cx0, &cy0, &cx1, &cy1);

                // push this frame's content UNION the previous frame's, so anything that
                // stopped being drawn -- or moved -- gets erased where it used to be
                int32_t bx0 = cx0, by0 = cy0, bx1 = cx1, by1 = cy1;
                if(panel_box_valid) {
                        if(panel_bx0 < bx0) bx0 = panel_bx0;
                        if(panel_by0 < by0) by0 = panel_by0;
                        if(panel_bx1 > bx1) bx1 = panel_bx1;
                        if(panel_by1 > by1) by1 = panel_by1;
                }
                panel_bx0 = cx0; panel_by0 = cy0;
                panel_bx1 = cx1; panel_by1 = cy1;
                panel_box_valid = true;

                if(bx0 < 0) bx0 = 0;
                if(by0 < 0) by0 = 0;
                if(bx1 > (int32_t)render->dim_x - 1) bx1 = render->dim_x - 1;
                if(by1 > (int32_t)render->dim_y - 1) by1 = render->dim_y - 1;

                for(uint8_t i = 0; i < ST_DISPLAY_COUNT; i++) {
                        light_display_command_update_region_async(_display[i],
                                (rend_point2d) {(uint16_t)bx0, (uint16_t)by0},
                                (rend_point2d) {(uint16_t)bx1, (uint16_t)by1});
                }
        }

        last_run = now;
        return LF_STATUS_RUN;
}

static void screentest_set_frame_rate(uint32_t frame_rate)
{
        if(frame_rate > 0)
                frame_interval_ms = 1000 / frame_rate;
}