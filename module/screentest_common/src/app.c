#include <screentest.h>
#include <light_platform.h>
#include <module/mod_light_display.h>

#include "screentest_internal.h"

static struct rend_context *render;
struct display_device *_display[ST_DISPLAY_COUNT];

static void screentest_event(const struct light_module *module, uint8_t event);
static uint8_t screentest_main(struct light_application *app);
static void screentest_set_frame_rate(uint32_t frame_rate);

void __screentest_hardware_init();

// app: screentest_po13
// defines two pico-oled-1.3 displays, one attached to the default pins
// for the display board, and one using spi port 0 and pins 16-20

Light_Application_Define(screentest, screentest_event, screentest_main,
                                &rend,
                                &light_display,
                                &light_platform,
                                &light_framework);

static uint32_t last_run;
static uint32_t next_frame;
static uint32_t frame_interval_ms;
static uint32_t frame_counter;

static uint8_t seq_counter;
static uint8_t seq_wrap;

void main()
{
        light_framework_init();
        light_framework_run();

}
  
static void screentest_event(const struct light_module *module, uint8_t event)
{
        switch(event) {
        case LF_EVENT_LOAD:;
                render = rend_context_create(
                        "screentest_render_main", 128, 64, 1);
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
                light_info("display pipeline setup complete","");
        break;
        // TODO implement unregister for event hooks
        case LF_EVENT_UNLOAD:
        break; 
        }
}
static uint8_t screentest_main(struct light_application *app)
{
        uint32_t now = light_platform_get_system_time_ms();
        light_info("enter Screentest application task, time=%dms, time since last run=%dms", now, last_run - now);

        if(now >= next_frame) {
                next_frame += frame_interval_ms;
                frame_counter++;
                seq_counter++;
                seq_counter %= seq_wrap;
//              rend_draw_point(display->render_ctx, (rend_point2d) {64, 32});
                rend_draw_circle(render, (rend_point2d) {64, 32}, 2 * (seq_counter + 1), true);
//              rend_debug_buffer_print_stdout(display->render_ctx);

                for(uint8_t i = 0; i < ST_DISPLAY_COUNT; i++) {
                        light_display_command_update(_display[i]);
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