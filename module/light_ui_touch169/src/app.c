#include <light_ui_demo.h>
#include <light_backlight.h>
#include <light_imu.h>
#include <light_touch.h>
#include <module/mod_light_audio.h>
#include <module/mod_light_backlight.h>
#include <module/mod_light_imu.h>
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
Light_Application_Define(light_ui_touch169, light_ui_demo_event, light_ui_demo_main,
                                &rend,
                                &light_display,
                                &light_ui,
                                &light_touch,
                                &light_imu,
                                &light_audio,
                                &light_backlight,
                                &light_core);

static struct touch_device *_touch_main;
// this app's own, not shared through light_ui_demo.h: the shared demo body never touches
// the IMU, only this board's orientation wiring does
static struct imu_device *_imu_main;
// down-edge detector. light_touch reports gestures on release, but a button press should
// land the moment the finger arrives, so this watches the touch state directly rather than
// going through light_touch_take_gesture()
static bool _touch_was_active;

void main(int argc, char **argv)
{
        light_framework_init();
        light_framework_run(argc, argv);
}

const rend_font_t *__light_ui_demo_font(void)
{
        // 16px here, where the po13 rig needs 8px: this panel is 240x280, so a 12x19 glyph
        // still leaves twenty characters per row and four comfortable button rows
        return &TypeLightSans_ttf_16px_font;
}

void __light_ui_demo_hardware_init(void)
{
        _display[0] = screentest_hw_ws_touch169_display();
        _touch_main = screentest_hw_ws_touch169_touch();
        _imu_main = screentest_hw_ws_touch169_imu();
        _backlight_main = screentest_hw_ws_touch169_backlight();
        // NULL until the board header names the buzzer's pin, which every use in the shared
        // demo already guards for
        _audio_main = screentest_hw_ws_touch169_audio();
}

// keeps the interface upright as the board is turned. light_ui knows nothing about IMUs --
// it takes a rotation -- so the translation lives here, and the board header owns the actual
// orientation-to-rotation table because it depends on the panel's native orientation
static void _poll_orientation(void)
{
        uint8_t orientation;
        if(!_imu_main || !light_imu_take_orientation(_imu_main, &orientation))
                return;

        // a settled orientation report means the board was picked up and turned, which is
        // someone handling it -- so it counts as activity even for the flat orientations
        // below that deliberately leave the rotation alone
        light_ui_demo_note_activity();

        uint8_t rotation;
        switch(orientation) {
        case IMU_ORIENT_PORTRAIT:       rotation = ST_IMU_ROTATION_PORTRAIT;      break;
        case IMU_ORIENT_PORTRAIT_FLIP:  rotation = ST_IMU_ROTATION_PORTRAIT_FLIP; break;
        case IMU_ORIENT_LANDSCAPE_L:    rotation = ST_IMU_ROTATION_LANDSCAPE_L;   break;
        case IMU_ORIENT_LANDSCAPE_R:    rotation = ST_IMU_ROTATION_LANDSCAPE_R;   break;
        default:
                // FACE_UP/FACE_DOWN: the board is flat and has no upright direction, so
                // hold whatever rotation it had rather than snapping to a default every
                // time it is set down
                light_info("orientation %d (flat) -- holding rotation", orientation);
                return;
        }
        // logged because the orientation-to-rotation table is the part most likely to need
        // correcting on a new board, and guessing from how the screen looks is slower than
        // reading which value produced which rotation
        light_info("orientation %d -> rotation %d", orientation, rotation);
        light_ui_set_rotation(_ui, rotation);
}

void __light_ui_demo_input_poll(void)
{
        _poll_orientation();

        if(!_touch_main)
                return;

        // light_touch's own periodic task keeps touch_active/x/y current, so this reads the
        // state it maintains rather than polling the controller a second time -- the
        // CST816T only answers for a short window after asserting its interrupt line, so a
        // redundant poll would mostly just return nothing anyway.
        //
        // the panel's own coordinates go straight through: light_ui_input_press_at() takes
        // them in the display's physical frame and untransforms them itself, which is what
        // keeps taps landing on the right widget once the UI has been rotated
        bool active = _touch_main->touch_active;
        if(active && !_touch_was_active) {
                // activity is noted for ANY touch, not only one that lands on a widget:
                // tapping a blank part of a dimmed screen is still someone asking for it,
                // and having to hit a button to wake the panel would be perverse
                light_ui_demo_note_activity();
                light_ui_input_press_at(_ui, _touch_main->x, _touch_main->y);
        }
        _touch_was_active = active;
}
