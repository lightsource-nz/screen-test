#include <screentest.h>
#include <light_platform.h>
#include <module/mod_light_display.h>
#include <module/mod_light_display_sh1106.h>

static struct rend_context *render;
static struct display_device *display[ST_DISPLAY_COUNT];

static void screentest_event(const struct light_module *module, uint8_t event, void *arg);
static uint8_t screentest_main(struct light_application *app);
static void screentest_set_frame_rate(uint32_t frame_rate);

// app: screentest_sh1106_i2c
// defines one generic 128x64 display with sh1106 driver, on I2C port 0 -- for the second
// physical panel (same SH1106 IC/glass, different breakout board that exposes I2C instead of
// SPI). see screentest_sh1106_spi4 for the SPI-wired board

Light_Application_Define(screentest, screentest_event, screentest_main,
                                &rend,
                                &light_display,
                                &light_display_sh1106,
                                &light_core);

static uint32_t last_run;
static uint32_t next_frame;
static uint32_t frame_interval_ms;
static uint32_t frame_counter;

void main(int argc, char **argv)
{
        light_framework_init();
        light_framework_run(argc, argv);

}

static void screentest_event(const struct light_module *module, uint8_t event, void *arg)
{
        switch(event) {
        case LF_EVENT_MODULE_LOAD:;
                render = rend_context_create(
                        "screentest_render_main", 128, 64, 1);
                render->point_radius = 2;
                frame_counter = 0;
                screentest_set_frame_rate(24);
                struct io_context *io_main =
                        light_ioport_setup_io_i2c(
                                ST_DISPLAY_0_PORT_ID,
                                ST_DISPLAY_0_PIN_RESET,
                                ST_DISPLAY_0_I2C_ADDR,
                                ST_DISPLAY_0_PIN_SCL,
                                ST_DISPLAY_0_PIN_SDA);
                struct display_device *disp_main =
                        light_display_sh1106_create_device(
                                "screentest_display_main", 128, 64, 1, io_main);
                light_display_set_render_context(disp_main, render);
                display[0] = disp_main;
                light_info("display pipeline setup complete","");
        break;
        // TODO implement unregister for event hooks
        case LF_EVENT_MODULE_UNLOAD:;
        break;
        }
}
static uint8_t screentest_main(struct light_application *app)
{
        uint32_t now = light_platform_get_time_since_init();
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
