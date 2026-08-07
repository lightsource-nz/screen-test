#include <screentest.h>
#include <light_platform.h>
#include <module/mod_light_display.h>
#include <module/mod_light_touch.h>

#include "screentest_internal.h"

static struct rend_context *render;
struct display_device *_display[ST_DISPLAY_COUNT];
struct touch_device *_touch_main;

// touch-triggered ripple: an outlined circle appears centered on the touch point on the
// down-edge of a touch, grows outward each frame, then disappears once it exceeds the
// max radius. _touch_main is NULL on boards with no touch hardware, so this is inert
// there -- touch_was_active never becomes true, and touch_anim_active never gets set
#define TOUCH_ANIM_START_RADIUS         4
#define TOUCH_ANIM_GROWTH               4
#define TOUCH_ANIM_MAX_RADIUS           40

static bool touch_was_active;
static bool touch_anim_active;
static uint16_t touch_anim_x, touch_anim_y, touch_anim_radius;
// whether a ripple was drawn into the frame that is currently on the panel. the frame
// after a ripple ends still has to repaint where it used to be in order to erase it, so
// the update region has to keep covering it for one frame longer than it is drawn
static bool touch_anim_on_panel;

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

static uint8_t seq_counter;
static uint8_t seq_wrap;

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
                seq_counter = 0;
                seq_wrap = 8;
                screentest_set_frame_rate(2);
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

        // checked every tick, not gated by the frame timer below -- light_touch's own
        // periodic task keeps _touch_main->touch_active/x/y fresh independently of this
        // app's frame rate, so throttling this check too could miss a brief touch
        bool touch_active_now = _touch_main && _touch_main->touch_active;
        if(touch_active_now && !touch_was_active) {
                touch_anim_active = true;
                touch_anim_x = _touch_main->x;
                touch_anim_y = _touch_main->y;
                touch_anim_radius = TOUCH_ANIM_START_RADIUS;
        }
        touch_was_active = touch_active_now;

        // the buffer we're about to swap into is the one that was in flight two frames
        // ago -- if some device is still flushing from it, wait rather than start
        // drawing over data a DMA transfer is still reading (see
        // rend_context_swap_buffers()/light_display_render_context_busy())
        if(now >= next_frame && !light_display_render_context_busy(render)) {
                next_frame += frame_interval_ms;
                frame_counter++;
                seq_counter++;
                seq_counter %= seq_wrap;
                rend_context_swap_buffers(render);
                rend_draw_clear(render);
//              rend_draw_point(display->render_ctx, (rend_point2d) {ST_RENDER_CIRCLE_X, ST_RENDER_CIRCLE_Y});
                rend_draw_circle(render, (rend_point2d) {ST_RENDER_CIRCLE_X, ST_RENDER_CIRCLE_Y}, 2 * (seq_counter + 1), true);
//              rend_debug_buffer_print_stdout(display->render_ctx);

                bool ripple_drawn = touch_anim_active;
                if(touch_anim_active) {
                        rend_draw_circle(render, (rend_point2d) {touch_anim_x, touch_anim_y}, touch_anim_radius, false);
                        touch_anim_radius += TOUCH_ANIM_GROWTH;
                        if(touch_anim_radius > TOUCH_ANIM_MAX_RADIUS)
                                touch_anim_active = false;
                }

                // the whole buffer was cleared and redrawn above, but the only pixels that
                // can actually differ from what the panel already shows are the two
                // animated circles -- so only a box covering those is pushed. the circles'
                // MAX radii are used rather than their current ones, since the region also
                // has to cover erasing whatever was drawn in the previous frame
                int32_t bx0 = INT32_MAX, by0 = INT32_MAX, bx1 = INT32_MIN, by1 = INT32_MIN;
                _bbox_add_circle(ST_RENDER_CIRCLE_X, ST_RENDER_CIRCLE_Y, 2 * seq_wrap,
                                &bx0, &by0, &bx1, &by1);
                if(ripple_drawn || touch_anim_on_panel) {
                        _bbox_add_circle(touch_anim_x, touch_anim_y, TOUCH_ANIM_MAX_RADIUS,
                                        &bx0, &by0, &bx1, &by1);
                }
                touch_anim_on_panel = ripple_drawn;

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