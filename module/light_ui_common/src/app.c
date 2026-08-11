#include <light_ui_demo.h>
#include <light_canvas.h>
#include <light_platform.h>

#include <stdint.h>

// shared light_ui demo: one framed window filling the canvas, with a stack of buttons in
// it. each button toggles its own label between "Name" and "Name *" when activated, which
// is enough to show a press landing on the right widget without needing screen space for a
// status line the 128x64 rigs don't have to spare.
//
// deliberately identical on both bring-up rigs, so the only thing that differs between a
// two-button board and a touch board is which input path drives the same widget tree

struct display_device *_display[LIGHT_UI_DEMO_DISPLAY_COUNT];
struct ui_context *_ui;
struct backlight_device *_backlight_main;

// when input was last seen, and whether the backlight has already been dimmed for idleness.
// the flag matters: without it every tick past the threshold would restart the fade, which
// would hold the brightness at its starting value forever
static uint32_t last_activity_ms;
static bool backlight_dimmed;

void light_ui_demo_note_activity(void)
{
        last_activity_ms = light_platform_get_time_since_init();
        if(!_backlight_main || !backlight_dimmed)
                return;
        backlight_dimmed = false;
        light_backlight_fade_to(_backlight_main, LIGHT_UI_DEMO_BACKLIGHT_FULL, LIGHT_UI_DEMO_FADE_UP_MS);
}

static void _service_idle_backlight(uint32_t now)
{
        if(!_backlight_main || backlight_dimmed)
                return;
        if(now - last_activity_ms < LIGHT_UI_DEMO_IDLE_MS)
                return;
        backlight_dimmed = true;
        light_backlight_fade_to(_backlight_main, LIGHT_UI_DEMO_BACKLIGHT_DIM, LIGHT_UI_DEMO_FADE_DOWN_MS);
}

#define LIGHT_UI_DEMO_BUTTON_COUNT              3

static const uint8_t *const _label_off[LIGHT_UI_DEMO_BUTTON_COUNT] = {
        (const uint8_t *)"Alpha", (const uint8_t *)"Beta", (const uint8_t *)"Gamma"
};
static const uint8_t *const _label_on[LIGHT_UI_DEMO_BUTTON_COUNT] = {
        (const uint8_t *)"Alpha *", (const uint8_t *)"Beta *", (const uint8_t *)"Gamma *"
};
static bool _toggled[LIGHT_UI_DEMO_BUTTON_COUNT];
static struct ui_button *_button[LIGHT_UI_DEMO_BUTTON_COUNT];

static struct rend_context *render;
static struct canvas_context *canvas;

static void _on_press(struct ui_button *btn, void *user_data)
{
        uintptr_t index = (uintptr_t)user_data;
        _toggled[index] = !_toggled[index];
        light_ui_button_set_label(btn,
                        _toggled[index] ? _label_on[index] : _label_off[index]);
        light_info("button %d toggled %s", (int)index, _toggled[index] ? "on" : "off");
}

static void _build_ui(void)
{
        struct ui_window *win = light_ui_window_create(_ui, NULL,
                (struct ui_rect) { 0, 0, (int16_t)render->dim_x - 1, (int16_t)render->dim_y - 1 },
                (const uint8_t *)LIGHT_UI_DEMO_TITLE);

        // before the children exist, so the one layout pass below already accounts for the
        // clearance the curve needs -- setting it afterwards would lay the stack out twice
        light_ui_window_set_corner_radius(win, LIGHT_UI_DEMO_CORNER_RADIUS);

        // rects are left at zero here: light_ui_window_layout_stack() below assigns every
        // child an equal-height row inside the window's content area, which is the whole
        // point of it existing -- nothing about this demo needs hand-placed geometry
        for(uintptr_t i = 0; i < LIGHT_UI_DEMO_BUTTON_COUNT; i++) {
                _toggled[i] = false;
                _button[i] = light_ui_button_create(_ui, &win->widget,
                                (struct ui_rect) { 0, 0, 0, 0 },
                                _label_off[i], _on_press, (void *)i);
        }
        light_ui_window_layout_stack(win, LIGHT_UI_DEMO_ROW_GAP);
}

void light_ui_demo_event(const struct light_module *module, uint8_t event, void *arg)
{
        switch(event) {
        case LF_EVENT_MODULE_LOAD:;
                render = rend_context_create("light_ui_demo_render",
                                LIGHT_UI_DEMO_RENDER_WIDTH, LIGHT_UI_DEMO_RENDER_HEIGHT, LIGHT_UI_DEMO_RENDER_BPP);
                rend_context_set_rotation(render, LIGHT_UI_DEMO_RENDER_ROTATION);
                // must happen before any widget is created: light_ui reads char_width/
                // char_height off the context to lay out and truncate labels, and
                // rend_draw_text() is a silent no-op with no font set
                rend_context_set_font(render, __light_ui_demo_font());

                light_debug("passing control to board hardware setup function","");
                __light_ui_demo_hardware_init();
                for(uint8_t i = 0; i < LIGHT_UI_DEMO_DISPLAY_COUNT; i++)
                        light_display_set_render_context(_display[i], render);

                // frame pacing, double buffering and region flushing live in the canvas;
                // light_ui contributes only the widget tree and what changed in it
                canvas = light_canvas_create(render, _display, LIGHT_UI_DEMO_DISPLAY_COUNT);
                light_canvas_enable_double_buffer(canvas);
                light_canvas_set_frame_rate(canvas, LIGHT_UI_DEMO_FRAME_RATE);

                _ui = light_ui_create_context(canvas);
                _build_ui();
                // after the tree exists, not before: setting the inset re-lays-out, and with
                // no root yet there would be nothing to lay out -- the window would keep the
                // full-canvas rect it was created with until the first rotation happened to
                // correct it
                light_ui_set_safe_inset(_ui, LIGHT_UI_DEMO_SAFE_INSET);
                // nothing on the panel matches the freshly built tree yet, so the first
                // frame has to push the whole canvas rather than just what changed
                light_ui_invalidate(_ui);
                light_info("ui pipeline setup complete","");
        break;
        // TODO implement unregister for event hooks
        case LF_EVENT_MODULE_UNLOAD:
        break;
        }
}

uint8_t light_ui_demo_main(struct light_application *app)
{
        // every tick, not once per frame: the input modules' own periodic tasks run
        // independently of this app's frame rate and hold only one event at a time, so
        // collecting at frame rate could drop one
        __light_ui_demo_input_poll();

        // after the input poll, so a tick that saw activity resets the timer before it is
        // tested rather than dimming for one tick and immediately waking again
        _service_idle_backlight(light_platform_get_time_since_init());

        // also every tick -- the canvas's own pacing decides when a frame actually happens,
        // and light_ui_render() is a no-op on the ticks in between (and on any tick where
        // nothing in the tree changed)
        light_ui_render(_ui);

        return LF_STATUS_RUN;
}
