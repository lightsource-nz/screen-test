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
struct audio_device *_audio_main;

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

// named once and used in three places -- the descriptor below, and both label arrays -- so a
// renamed button cannot end up disagreeing with the label it toggles back to
#define DEMO_LABEL_0                            "Alpha"
#define DEMO_LABEL_1                            "Beta"
#define DEMO_LABEL_2                            "Gamma"

static const uint8_t *const _label_off[LIGHT_UI_DEMO_BUTTON_COUNT] = {
        (const uint8_t *)DEMO_LABEL_0, (const uint8_t *)DEMO_LABEL_1, (const uint8_t *)DEMO_LABEL_2
};
// parenthesised so the cast plainly applies to the concatenated literal. it would anyway --
// adjacent string literals are joined in translation phase 6, before the cast is parsed -- but
// unparenthesised it reads like a cast of the first literal alone, next to a stray second one
static const uint8_t *const _label_on[LIGHT_UI_DEMO_BUTTON_COUNT] = {
        (const uint8_t *)(DEMO_LABEL_0 " *"), (const uint8_t *)(DEMO_LABEL_1 " *"),
        (const uint8_t *)(DEMO_LABEL_2 " *")
};
static bool _toggled[LIGHT_UI_DEMO_BUTTON_COUNT];

static struct light_draw_context *render;
static struct canvas_context *canvas;

static void _on_press(struct ui_button *btn, void *user_data)
{
        uintptr_t index = (uintptr_t)user_data;
        _toggled[index] = !_toggled[index];
        light_ui_button_set_label(btn,
                        _toggled[index] ? _label_on[index] : _label_off[index]);
        light_info("button %d toggled %s", (int)index, _toggled[index] ? "on" : "off");
        if(_audio_main)
                light_audio_tone(_audio_main, LIGHT_UI_DEMO_CLICK_HZ, LIGHT_UI_DEMO_CLICK_MS);
}

// a short decaying chirp, synthesised rather than embedded: it exercises conversion, DMA and
// rate pacing with nothing to build, embed or keep in flash, which is the whole point of a
// bring-up sound. a square wave rather than a sine because the maths stays integer and a
// piezo cannot tell the difference anyway
static void _play_startup_chirp(void)
{
        if(!_audio_main)
                return;

        uint32_t count = (LIGHT_UI_DEMO_CHIRP_RATE * LIGHT_UI_DEMO_CHIRP_MS) / 1000;
        int16_t *pcm = light_alloc(count * sizeof(int16_t));
        if(!pcm) {
                light_warn("no memory for the startup chirp (%d samples)", (int)count);
                return;
        }

        // sweeps upward, with a linear decay so it ends at silence -- stopping at full
        // amplitude would leave a step, and a step is a click.
        //
        // amplitude is a config knob (LIGHT_UI_DEMO_CHIRP_AMPLITUDE) rather than a constant
        // because the right value depends entirely on what is on the end of the pin. Raising
        // it to full scale was tried on the touch board's piezo and changed nothing audible,
        // so the default sits below maximum where it leaves headroom for a board with an
        // amplifier
        uint32_t phase = 0;
        for(uint32_t i = 0; i < count; i++) {
                uint32_t hz = 1200 + (2400 * i) / count;
                phase += hz;
                int32_t amp = LIGHT_UI_DEMO_CHIRP_AMPLITUDE
                                - (int32_t)((LIGHT_UI_DEMO_CHIRP_AMPLITUDE * (int64_t)i) / count);
                bool high = ((phase / LIGHT_UI_DEMO_CHIRP_RATE) & 1) != 0;
                pcm[i] = (int16_t)(high ? amp : -amp);
        }

        struct audio_format fmt = {
                .sample_rate = LIGHT_UI_DEMO_CHIRP_RATE,
                .encoding = LIGHT_AUDIO_PCM_S16,
                .channels = 1
        };
        if(!light_audio_play_pcm(_audio_main, pcm, count, fmt))
                light_warn("startup chirp was refused by the audio device","");
        // light_audio_play_pcm() converts into its own buffer for S16, so this one has done
        // its job by the time the call returns
        light_free(pcm);
}

// the whole interface, as data. the tree's shape is the source's shape: three buttons, in the
// order they appear on screen, inside one rounded window that stacks them.
//
// no rects anywhere -- Light_UI_Stack() gives every child an equal-height row in the window's
// content area, which is the entire reason light_ui_window_layout_stack() exists. the window
// gets no rect either: it is the root, and light_ui_relayout() sizes the root to the canvas,
// which is what makes this identical source work on a 64x128 OLED and a 240x280 panel.
//
// the user_data index is what ties a press back to _toggled[]/_label_on[]; _on_press receives
// the button itself, so nothing here needs to be bound to a variable (the old code kept a
// _button[] array that was written and never read)
Light_UI_Button_Define(_btn_alpha, DEMO_LABEL_0, _on_press, (void *)0);
Light_UI_Button_Define(_btn_beta,  DEMO_LABEL_1, _on_press, (void *)1);
Light_UI_Button_Define(_btn_gamma, DEMO_LABEL_2, _on_press, (void *)2);

#if LIGHT_UI_DEMO_PAGES

//   the navigation example. Two pages: the main one, and a second reached from its last row
// and returned from with a swipe. The pages reference each other -- the child names its
// parent, the parent's handler names the child -- so the child is declared before the handler
// that navigates to it
Light_UI_Page_Declare(_page_detail);

static void _on_open_detail(struct ui_button *btn, void *user_data)
{
        if(_audio_main)
                light_audio_tone(_audio_main, LIGHT_UI_DEMO_CLICK_HZ, LIGHT_UI_DEMO_CLICK_MS);
        //   last statement in the handler, and deliberately: navigating destroys the tree this
        // button belongs to, so `btn` is released memory the moment it returns
        light_ui_navigate(btn->widget.ui, &_page_detail);
}
static void _on_detail_back(struct ui_button *btn, void *user_data)
{
        if(_audio_main)
                light_audio_tone(_audio_main, LIGHT_UI_DEMO_CLICK_HZ, LIGHT_UI_DEMO_CLICK_MS);
        // the same route the swipe takes, so the on-screen control and the gesture cannot
        // disagree about where "back" is
        light_ui_navigate_back(btn->widget.ui);
}

Light_UI_Button_Define(_btn_more, "More >", _on_open_detail, NULL);

Light_UI_Window_Define(_demo_window, LIGHT_UI_DEMO_TITLE,
        Light_UI_Rounded(LIGHT_UI_DEMO_CORNER_RADIUS),
        Light_UI_Stack(LIGHT_UI_DEMO_ROW_GAP),
        Light_UI_Children(&_btn_alpha, &_btn_beta, &_btn_gamma, &_btn_more));

Light_UI_Label_Define(_lbl_detail, "swipe right to go back");
Light_UI_Button_Define(_btn_back, "< Back", _on_detail_back, NULL);
Light_UI_Window_Define(_detail_window, "More",
        Light_UI_Rounded(LIGHT_UI_DEMO_CORNER_RADIUS),
        Light_UI_Stack(LIGHT_UI_DEMO_ROW_GAP),
        Light_UI_Children(&_lbl_detail, &_btn_back));

// the main page is top-level, so back from it has nowhere to go and does nothing
Light_UI_Page_Define(_page_main, NULL, _demo_window);
Light_UI_Page_Define(_page_detail, &_page_main, _detail_window);

#else

Light_UI_Window_Define(_demo_window, LIGHT_UI_DEMO_TITLE,
        Light_UI_Rounded(LIGHT_UI_DEMO_CORNER_RADIUS),
        Light_UI_Stack(LIGHT_UI_DEMO_ROW_GAP),
        Light_UI_Children(&_btn_alpha, &_btn_beta, &_btn_gamma));

#endif

void light_ui_demo_event(const struct light_module *module, uint8_t event, void *arg)
{
        switch(event) {
        case LF_EVENT_MODULE_LOAD:;
                render = light_draw_context_create("light_ui_demo_render",
                                LIGHT_UI_DEMO_RENDER_WIDTH, LIGHT_UI_DEMO_RENDER_HEIGHT, LIGHT_UI_DEMO_RENDER_BPP);
                light_draw_context_set_rotation(render, LIGHT_UI_DEMO_RENDER_ROTATION);
                // must happen before any widget is created: light_ui reads char_width/
                // char_height off the context to lay out and truncate labels, and
                // light_draw_draw_text() is a silent no-op with no font set
                light_draw_context_set_font(render, __light_ui_demo_font());

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
                // the whole widget tree, from the descriptor above. building a root sizes it
                // to the canvas, so there is no rect to compute here
#if LIGHT_UI_DEMO_PAGES
                //   entered through the page system rather than built directly, so the context
                // knows which page it is showing. light_ui_build() on its own leaves ui->page
                // NULL, and navigate_back() would then have nothing to reason from
                light_ui_navigate(_ui, &_page_main);
#else
                light_ui_build(_ui, NULL, &_demo_window);
#endif
                // after the tree exists, not before: setting the inset re-lays-out, and with
                // no root yet there would be nothing to lay out
                light_ui_set_safe_inset(_ui, LIGHT_UI_DEMO_SAFE_INSET);
                // nothing on the panel matches the freshly built tree yet, so the first
                // frame has to push the whole canvas rather than just what changed
                light_ui_invalidate(_ui);
                light_info("ui pipeline setup complete","");
                // last, so a board where audio misbehaves has already got its display up --
                // a bring-up sound that hangs before the first frame would look like a dead
                // panel rather than an audio fault
                _play_startup_chirp();
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
