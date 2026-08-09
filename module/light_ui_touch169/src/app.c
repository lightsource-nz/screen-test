#include <screentest_ui.h>
#include <light_touch.h>
#include <module/mod_light_touch.h>
#include <module/mod_light_display.h>
#include <module/mod_light_ui.h>
#include <TypeLightSans_ttf_16px_font.h>

// app: light_ui_touch169
// the shared light_ui demo on the RP2350-Touch-LCD-1.69, driven by tapping buttons
// directly. no push-buttons and no light_button dependency -- this is the other half of the
// pair, proving the same widget tree works from either input path

// named per-app rather than after the shared demo: this define lives in each app precisely
// so it can name its own dependencies, and the name it gives the application is what
// light_module_get_name() reports in the log, so it should say which binary is running
Light_Application_Define(light_ui_touch169, screentest_ui_event, screentest_ui_main,
                                &rend,
                                &light_display,
                                &light_ui,
                                &light_touch,
                                &light_core);

static struct touch_device *_touch_main;
// down-edge detector. light_touch reports gestures on release, but a button press should
// land the moment the finger arrives, so this watches the touch state directly rather than
// going through light_touch_take_gesture()
static bool _touch_was_active;

void main(int argc, char **argv)
{
        light_framework_init();
        light_framework_run(argc, argv);
}

const rend_font_t *__screentest_ui_font(void)
{
        // 16px here, where the po13 rig needs 8px: this panel is 240x280, so a 12x19 glyph
        // still leaves twenty characters per row and four comfortable button rows
        return &TypeLightSans_ttf_16px_font;
}

void __screentest_ui_hardware_init(void)
{
        _display[0] = screentest_hw_ws_touch169_display();
        _touch_main = screentest_hw_ws_touch169_touch();
}

void __screentest_ui_input_poll(void)
{
        if(!_touch_main)
                return;

        // light_touch's own periodic task keeps touch_active/x/y current, so this reads the
        // state it maintains rather than polling the controller a second time -- the
        // CST816T only answers for a short window after asserting its interrupt line, so a
        // redundant poll would mostly just return nothing anyway.
        //
        // coordinates go straight through: this app's render context is REND_ROTATE_0, so
        // the panel's physical space and light_ui's logical space are the same one (see
        // light_ui_input_press_at() for what a rotated context would need)
        bool active = _touch_main->touch_active;
        if(active && !_touch_was_active)
                light_ui_input_press_at(_ui, _touch_main->x, _touch_main->y);
        _touch_was_active = active;
}
