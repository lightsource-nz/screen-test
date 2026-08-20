#include <light_ui_hw_ws_touch169.h>
#include <light_canvas.h>
#include <light_backlight.h>
#include <light_display.h>
#include <light_platform.h>
#include <light_draw.h>
#include <module/mod_light_canvas.h>
#include <module/mod_light_display.h>
#include <module/mod_light_backlight.h>
#include <TypeLightSans_ttf_16px_font.h>

#include <stdio.h>

// app: screentest_calib169
//
// measures the panel's VISIBLE corner radius, which is not the same as the pixel grid's. the
// glass on this board is rounded, so the outermost pixels near each corner are not shown at
// all, and every attempt to put a frame near the panel edge has to know where that boundary
// actually is. ST_DISPLAY_CORNER_RADIUS started life as a guess, and a guess is what put the
// UI demo's frame corners outside the glass.
//
// HOW TO READ IT: a rounded rectangle is drawn at the same inset the UI demo uses, and its
// corner radius steps upward on a timer with the current value printed in the middle of the
// screen (and to the USB console). while the radius is too small the frame's corners are
// sharper than the glass and get clipped -- you see four straight edges that stop short and
// no curve joining them. the first value at which all four corners join up is the answer.
//
// WHY THAT VALUE IS DIRECTLY USEFUL: a rounded rect drawn at inset d with radius r sits
// entirely inside glass of radius R exactly when r >= R - d, because insetting the glass by d
// gives a rounded rect of radius R-d over the same bounds. so the number on screen when the
// corners first close IS LIGHT_UI_DEMO_CORNER_RADIUS, and R = that + the inset

// deliberately the same inset the UI demo draws its root frame at, so the radius read off
// here can be pasted straight into LIGHT_UI_DEMO_CORNER_RADIUS with no arithmetic
#define CALIB_INSET             2
#define CALIB_RADIUS_MIN        4
#define CALIB_RADIUS_MAX        76
#define CALIB_RADIUS_STEP       4
// long enough to look at all four corners before it moves on, short enough that a full sweep
// takes well under a minute
#define CALIB_DWELL_MS          2000
#define CALIB_FRAME_RATE        10

// forward declared because Light_Application_Define takes them by name and expands above
// their definitions -- the other apps get away without this only by living in a header
void screentest_calib169_event(const struct light_module *module, uint8_t event, void *arg);
uint8_t screentest_calib169_main(struct light_application *app);

Light_Application_Define(screentest_calib169, screentest_calib169_event, screentest_calib169_main,
                                &light_draw,
                                &light_display,
                                &light_canvas,
                                &light_backlight,
                                &light_core);

static struct display_device *_display;
static struct backlight_device *_backlight;
static struct light_draw_context *render;
static struct canvas_context *canvas;

static uint16_t _radius = CALIB_RADIUS_MIN;
static uint32_t _last_step_ms;

void main(int argc, char **argv)
{
        light_framework_init();
        light_framework_run(argc, argv);
}

static void _draw_pattern(void)
{
        light_draw_draw_clear(render);

        int16_t x0 = CALIB_INSET, y0 = CALIB_INSET;
        int16_t x1 = (int16_t)render->dim_x - 1 - CALIB_INSET;
        int16_t y1 = (int16_t)render->dim_y - 1 - CALIB_INSET;
        light_draw_draw_rect_rounded(render, (light_draw_point2d) { (uint16_t)x0, (uint16_t)y0 },
                                       (light_draw_point2d) { (uint16_t)x1, (uint16_t)y1 },
                                       _radius, false);

        // the number goes in the middle, where no corner can ever swallow it -- the whole
        // point is that it stays readable at radii where the corners do not
        uint8_t buf[8];
        snprintf((char *)buf, sizeof(buf), "%u", (unsigned)_radius);
        const light_draw_font_t *font = render->font;
        if(!font)
                return;
        size_t len = 0;
        while(buf[len])
                len++;
        int16_t tx = (int16_t)((render->dim_x - len * font->char_width) / 2);
        int16_t ty = (int16_t)((render->dim_y - font->char_height) / 2);
        light_draw_draw_text(render, (light_draw_point2d) { (uint16_t)tx, (uint16_t)ty }, buf);

        // short ticks on the centre lines, so it is obvious whether the straight edges
        // themselves are reaching the glass -- if even those are clipped, the problem is an
        // offset rather than a corner radius
        light_draw_draw_line(render, (light_draw_point2d) { (uint16_t)(render->dim_x / 2), (uint16_t)y0 },
                               (light_draw_point2d) { (uint16_t)(render->dim_x / 2), (uint16_t)(y0 + 10) }, true);
        light_draw_draw_line(render, (light_draw_point2d) { (uint16_t)(render->dim_x / 2), (uint16_t)(y1 - 10) },
                               (light_draw_point2d) { (uint16_t)(render->dim_x / 2), (uint16_t)y1 }, true);
        light_draw_draw_line(render, (light_draw_point2d) { (uint16_t)x0, (uint16_t)(render->dim_y / 2) },
                               (light_draw_point2d) { (uint16_t)(x0 + 10), (uint16_t)(render->dim_y / 2) }, true);
        light_draw_draw_line(render, (light_draw_point2d) { (uint16_t)(x1 - 10), (uint16_t)(render->dim_y / 2) },
                               (light_draw_point2d) { (uint16_t)x1, (uint16_t)(render->dim_y / 2) }, true);
}

void screentest_calib169_event(const struct light_module *module, uint8_t event, void *arg)
{
        switch(event) {
        case LF_EVENT_MODULE_LOAD:;
                render = light_draw_context_create("calib169_render",
                                ST_DISPLAY_WIDTH, ST_DISPLAY_HEIGHT, 16);
                light_draw_context_set_rotation(render, LIGHT_DRAW_ROTATE_0);
                light_draw_context_set_font(render, &TypeLightSans_ttf_16px_font);

                _display = light_ui_hw_ws_touch169_display();
                _backlight = light_ui_hw_ws_touch169_backlight();
                light_display_set_render_context(_display, render);

                canvas = light_canvas_create(render, &_display, 1);
                light_canvas_enable_double_buffer(canvas);
                light_canvas_set_frame_rate(canvas, CALIB_FRAME_RATE);

                // no idle dimming here on purpose: reading the corners means staring at the
                // panel without touching it, which is exactly what an idle timeout punishes
                if(_backlight)
                        light_backlight_set_level(_backlight, LIGHT_BACKLIGHT_LEVEL_MAX);

                _last_step_ms = light_platform_get_time_since_init();
                light_info("calib: inset %d, sweeping radius %d..%d step %d every %d ms",
                                CALIB_INSET, CALIB_RADIUS_MIN, CALIB_RADIUS_MAX,
                                CALIB_RADIUS_STEP, CALIB_DWELL_MS);
                light_info("calib: report the FIRST radius whose four corners join up","");
        break;
        case LF_EVENT_MODULE_UNLOAD:
        break;
        }
}

uint8_t screentest_calib169_main(struct light_application *app)
{
        uint32_t now = light_platform_get_time_since_init();
        if(now - _last_step_ms >= CALIB_DWELL_MS) {
                _last_step_ms = now;
                _radius += CALIB_RADIUS_STEP;
                if(_radius > CALIB_RADIUS_MAX)
                        _radius = CALIB_RADIUS_MIN;
                light_info("calib: radius %d", _radius);
        }

        if(light_canvas_frame_begin(canvas)) {
                _draw_pattern();
                // the whole panel every frame: this is a calibration target, not something
                // that has to be cheap, and a full push removes any doubt about whether a
                // missing corner is the glass or an unflushed region
                light_canvas_invalidate_all(canvas);
                light_canvas_frame_end(canvas);
        }
        return LF_STATUS_RUN;
}
