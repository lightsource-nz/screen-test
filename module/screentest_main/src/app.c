#include <screentest.h>

#include "screentest_internal.h"

static struct display_device *display;

void screentest_event(const struct light_module *module, uint8_t event);
static uint8_t screentest_main(struct light_application *app);

Light_Application_Define(screentest, screentest_event, screentest_main,
                                &rend,
                                &light_display,
                                &light_display_po13,
                                &light_framework);

void main()
{
        light_framework_init();
        light_framework_run();

}
  
void screentest_event(const struct light_module *module, uint8_t event)
{
        switch(event) {
                case LF_EVENT_LOAD:;
                        struct sh1107_io_context *io = light_display_po13_setup_io_spi_4p(PORT_SPI_1);
                        display = light_display_po13_create_device("screentest_display_main", io); 
                        struct rend_context *render = rend_context_create("screentest_render_main", 128, 64, 1);
                        light_display_set_render_context(display, render);
                        light_info("display pipeline setup complete","");
                break;
                // TODO implement unregister for event hooks
                case LF_EVENT_UNLOAD:
                break; 
        }
}
static uint8_t screentest_main(struct light_application *app)
{
        // TODO add tick counter and cycle timer, etc
        light_info("enter Screentest application task, time=%dms, time since last run=%dms", 0, 0);

        display->render_ctx->point_radius = 2;
//        rend_draw_point(display->render_ctx, (rend_point2d) {64, 32});
        rend_draw_circle(display->render_ctx, (rend_point2d) {64, 32}, 10, true);
        rend_debug_buffer_print_stdout(display->render_ctx);

        light_display_command_clear(display, 0);
        light_display_command_update(display);

        return LF_STATUS_SHUTDOWN;
}
