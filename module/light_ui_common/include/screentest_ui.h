#ifndef _SCREENTEST_UI_H
#define _SCREENTEST_UI_H

#include <light.h>
#include <light_backlight.h>
#include <light_display.h>
#include <light_ui.h>
#include <rend.h>

#include <stdint.h>

// TODO implement version fields properly
#define ST_UI_VERSION_STR               "0.1.0"

// default UI demo configuration, sized for the small portrait OLED rigs. apps with a
// differently-shaped display provide their own complete screentest_ui.h that shadows this
// one entirely -- the same convention (and the same include-order mechanism) that
// screentest_common's screentest.h already uses
#define ST_UI_DISPLAY_COUNT             1
#define ST_UI_RENDER_WIDTH              64
#define ST_UI_RENDER_HEIGHT             128
#define ST_UI_RENDER_BPP                1
#define ST_UI_RENDER_ROTATION           REND_ROTATE_90

#define ST_UI_TITLE                     "light_ui"
// pixels between stacked button rows
#define ST_UI_ROW_GAP                   2
// how often the UI is offered a chance to repaint. light_ui_render() is a no-op unless
// something actually changed, so this bounds latency after an input rather than describing
// a steady redraw load
#define ST_UI_FRAME_RATE                24

// how long without input before the backlight dims, and the levels and fade times either
// side of it. dimming rather than blanking: the UI stays readable, so it reads as the device
// resting rather than switching off
#define ST_UI_IDLE_MS                   8000
#define ST_UI_BACKLIGHT_FULL            LIGHT_BACKLIGHT_LEVEL_MAX
#define ST_UI_BACKLIGHT_DIM             150
// waking is faster than dimming on purpose -- a slow fade down is unobtrusive, a slow fade
// up feels unresponsive to the touch that asked for it
#define ST_UI_FADE_DOWN_MS              400
#define ST_UI_FADE_UP_MS                150

extern struct display_device *_display[ST_UI_DISPLAY_COUNT];
extern struct ui_context *_ui;
// NULL on boards with no controllable backlight, on the same terms as _touch_main in the
// circle demo -- the idle behaviour is simply skipped there
extern struct backlight_device *_backlight_main;

// called by an app whenever it sees user input, to hold off (or undo) the idle dim. the
// shared demo owns the timer; only the app knows what counts as input on its board
extern void screentest_ui_note_activity(void);

// --- provided by light_ui_common, referenced by each app's Light_Application_Define ---
// the module dependency list has to name the input modules the board actually has
// (light_button, light_touch, or neither), so each app owns its own application define and
// main(); everything else about the demo is shared
extern void screentest_ui_event(const struct light_module *module, uint8_t event, void *arg);
extern uint8_t screentest_ui_main(struct light_application *app);

// --- provided by each app ---
// creates the board's display (into _display[]) and whatever input devices it has
extern void __screentest_ui_hardware_init(void);
// called every tick: translate this board's input devices onto light_ui's hardware-free
// input entry points. a handful of lines either way -- see the two UI apps for the
// two-button and the touch wiring
extern void __screentest_ui_input_poll(void);
// the font labels are rendered in. rendered at build time by crush, whose generated symbol
// name embeds the pixel size -- and the right pixel size differs per panel -- so the font
// can't be named by shared code
extern const rend_font_t *__screentest_ui_font(void);

#endif
