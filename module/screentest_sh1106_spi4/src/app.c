#include <screentest.h>
#include <light_platform.h>
#include <module/mod_light_display.h>
#include <module/mod_light_display_sh1106.h>

#include "screentest_internal.h"

static struct rend_context *render;
static struct display_device *display[ST_DISPLAY_COUNT];

static void screentest_event(const struct light_module *module, uint8_t event);
static uint8_t screentest_main(struct light_application *app);
static void screentest_set_frame_rate(uint32_t frame_rate);

// app: screentest_sh1106_spi4
// defines one generic 128x64 display with sh1106 driver, on SPI port 0

Light_Application_Define(screentest, screentest_event, screentest_main,
                                &rend,
                                &light_display,
                                &light_display_sh1106,
                                &light_platform,
                                &light_framework);

static uint32_t last_run;
static uint32_t next_frame;
static uint32_t frame_interval_ms;
static uint32_t frame_counter;

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
                screentest_set_frame_rate(24);
                struct io_context *io_main =
                        light_display_ioport_setup_io_spi_4p(
                                ST_DISPLAY_0_PORT_ID,
                                ST_DISPLAY_0_PIN_RESET,
                                ST_DISPLAY_0_PIN_CS,
                                ST_DISPLAY_0_PIN_DC,
                                ST_DISPLAY_0_PIN_SCK,
                                ST_DISPLAY_0_PIN_TX);
                struct display_device *disp_main =
                        light_display_sh1106_create_device(
                                "screentest_display_main", 64, 128, 1, io_main);
                light_display_set_render_context(disp_main, render);
                display[0] = disp_main;
                light_info("display pipeline setup complete","");
        break;
        // TODO implement unregister for event hooks
        case LF_EVENT_UNLOAD:;
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
//              rend_draw_point(display->render_ctx, (rend_point2d) {64, 32});
                rend_draw_circle(render, (rend_point2d) {64, 32}, 10, true);
//              rend_debug_buffer_print_stdout(display->render_ctx);

                for(uint8_t i = 0; i < ST_DISPLAY_COUNT; i++) {
                        light_display_command_update(display[i]);
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