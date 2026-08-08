#include <screentest_ui.h>
#include <light_platform.h>

#include <stdint.h>

// shared light_ui demo: one framed window filling the canvas, with a stack of buttons in
// it. each button toggles its own label between "Name" and "Name *" when activated, which
// is enough to show a press landing on the right widget without needing screen space for a
// status line the 128x64 rigs don't have to spare.
//
// deliberately identical on both bring-up rigs, so the only thing that differs between a
// two-button board and a touch board is which input path drives the same widget tree

struct display_device *_display[ST_UI_DISPLAY_COUNT];
struct ui_context *_ui;

#define ST_UI_BUTTON_COUNT              3

static const uint8_t *const _label_off[ST_UI_BUTTON_COUNT] = {
        (const uint8_t *)"Alpha", (const uint8_t *)"Beta", (const uint8_t *)"Gamma"
};
static const uint8_t *const _label_on[ST_UI_BUTTON_COUNT] = {
        (const uint8_t *)"Alpha *", (const uint8_t *)"Beta *", (const uint8_t *)"Gamma *"
};
static bool _toggled[ST_UI_BUTTON_COUNT];
static struct ui_button *_button[ST_UI_BUTTON_COUNT];

static struct rend_context *render;
static uint32_t next_frame;
static uint32_t frame_interval_ms;

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
                (const uint8_t *)ST_UI_TITLE);

        // rects are left at zero here: light_ui_window_layout_stack() below assigns every
        // child an equal-height row inside the window's content area, which is the whole
        // point of it existing -- nothing about this demo needs hand-placed geometry
        for(uintptr_t i = 0; i < ST_UI_BUTTON_COUNT; i++) {
                _toggled[i] = false;
                _button[i] = light_ui_button_create(_ui, &win->widget,
                                (struct ui_rect) { 0, 0, 0, 0 },
                                _label_off[i], _on_press, (void *)i);
        }
        light_ui_window_layout_stack(win, ST_UI_ROW_GAP);
}

void screentest_ui_event(const struct light_module *module, uint8_t event, void *arg)
{
        switch(event) {
        case LF_EVENT_MODULE_LOAD:;
                render = rend_context_create("screentest_ui_render_main",
                                ST_UI_RENDER_WIDTH, ST_UI_RENDER_HEIGHT, ST_UI_RENDER_BPP);
                rend_context_set_rotation(render, ST_UI_RENDER_ROTATION);
                // must happen before any widget is created: light_ui reads char_width/
                // char_height off the context to lay out and truncate labels, and
                // rend_draw_text() is a silent no-op with no font set
                rend_context_set_font(render, __screentest_ui_font());
                rend_context_enable_double_buffer(render);
                if(ST_UI_FRAME_RATE > 0)
                        frame_interval_ms = 1000 / ST_UI_FRAME_RATE;

                light_debug("passing control to board hardware setup function","");
                __screentest_ui_hardware_init();
                for(uint8_t i = 0; i < ST_UI_DISPLAY_COUNT; i++)
                        light_display_set_render_context(_display[i], render);

                _ui = light_ui_create_context(render, _display, ST_UI_DISPLAY_COUNT);
                _build_ui();
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

uint8_t screentest_ui_main(struct light_application *app)
{
        uint32_t now = light_platform_get_time_since_init();

        // every tick, not once per frame: the input modules' own periodic tasks run
        // independently of this app's frame rate and hold only one event at a time, so
        // collecting at frame rate could drop one
        __screentest_ui_input_poll();

        if(now >= next_frame) {
                // set forward from now rather than accumulated, so a long stall doesn't
                // leave a backlog of frame deadlines to burn through
                next_frame = now + frame_interval_ms;
                light_ui_render(_ui);
        }

        return LF_STATUS_RUN;
}
