#include <screentest.h>
#include <light_platform.h>
#include <module/mod_light_display.h>

#include "screentest_internal.h"

static struct rend_context *render;
struct display_device *_display[ST_DISPLAY_COUNT];

static void screentest_event(const struct light_module *module, uint8_t event, void *arg);
static uint8_t screentest_main(struct light_application *app);
static void screentest_set_frame_rate(uint32_t frame_rate);

void __screentest_hardware_init();

Light_Application_Define(screentest, screentest_event, screentest_main,
                                &rend,
                                &light_display,
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

                for(uint8_t i = 0; i < ST_DISPLAY_COUNT; i++) {
                        light_display_command_update_async(_display[i]);
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