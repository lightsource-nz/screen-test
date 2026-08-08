#ifndef _SCREENTEST_UI_H
#define _SCREENTEST_UI_H

#include <light.h>
#include <light_display.h>
#include <light_ui.h>
#include <rend.h>
// board pinout and device construction (ST_DISPLAY_*, ST_TOUCH_*)
#include <screentest_hw_ws_touch169.h>

#include <stdint.h>

// this header shadows screentest_ui_common's entirely, so everything the shared demo reads
// has to be repeated here rather than inherited -- same convention as screentest.h

#define ST_UI_VERSION_STR               "0.1.0"

#define ST_UI_DISPLAY_COUNT             1

// this panel's real geometry, rather than the small OLED rigs' 64x128 1bpp default. no
// rotation: the ST7789 driver already declares width/height in the panel's own native
// orientation, so the logical and physical spaces coincide -- which is also what lets touch
// coordinates be passed straight to light_ui without an inverse transform
#define ST_UI_RENDER_WIDTH              ST_DISPLAY_WIDTH
#define ST_UI_RENDER_HEIGHT             ST_DISPLAY_HEIGHT
#define ST_UI_RENDER_BPP                16
#define ST_UI_RENDER_ROTATION           REND_ROTATE_0

#define ST_UI_TITLE                     "light_ui"
// a roomier gap than the OLED rigs' 2px: rows here are ~60px tall, so 2px would read as
// the buttons being fused together
#define ST_UI_ROW_GAP                   6
#define ST_UI_FRAME_RATE                24

extern struct display_device *_display[ST_UI_DISPLAY_COUNT];
extern struct ui_context *_ui;

extern void screentest_ui_event(const struct light_module *module, uint8_t event, void *arg);
extern uint8_t screentest_ui_main(struct light_application *app);

extern void __screentest_ui_hardware_init(void);
extern void __screentest_ui_input_poll(void);
extern const rend_font_t *__screentest_ui_font(void);

#endif
