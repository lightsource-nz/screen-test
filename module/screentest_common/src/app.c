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
                // both test displays (po13 and the second raw SH1107 device) are physically
                // 64 wide x 128 tall portrait panels -- create the render context at those
                // physical dimensions and rotate it, rather than creating it pre-rotated
                // (128x64) with no rotation transform, which mismatches the actual device
                // buffers and scrambles pixel positions (matches crossfire's own working
                // setup for the same panel -- see crossfire.c's crossfire_display_init())
                render = rend_context_create(
                        "screentest_render_main", 64, 128, 1);
                rend_context_set_rotation(render, REND_ROTATE_90);
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
        light_info("enter Screentest application task, time=%dms, time since last run=%dms", now, last_run - now);

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
//              rend_draw_point(display->render_ctx, (rend_point2d) {64, 32});
                rend_draw_circle(render, (rend_point2d) {64, 32}, 2 * (seq_counter + 1), true);
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