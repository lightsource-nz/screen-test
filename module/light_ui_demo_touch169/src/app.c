#include <light_ui_demo.h>
#include <light_cli.h>
#include <light_backlight.h>
#include <light_imu.h>
#include <light_touch.h>
#include <module/mod_light_audio.h>
#include <module/mod_light_backlight.h>
#include <module/mod_light_imu.h>
#include <module/mod_light_touch.h>
#include <module/mod_light_display.h>
#include <module/mod_light_ui.h>
#include <module/mod_light_cli.h>
#include <TypeLightSans_ttf_16px_font.h>

// app: light_ui_demo_touch169
// the shared light_ui demo on the RP2350-Touch-LCD-1.69, driven by tapping buttons
// directly. no push-buttons and no light_button dependency -- this is the other half of the
// pair, proving the same widget tree works from either input path

// named per-app rather than after the shared demo: this define lives in each app precisely
// so it can name its own dependencies, and the name it gives the application is what
// light_module_get_name() reports in the log, so it should say which binary is running
Light_Application_Define(light_ui_demo_touch169, light_ui_demo_event, light_ui_demo_main,
                                &rend,
                                &light_display,
                                &light_ui,
                                &light_touch,
                                &light_imu,
                                &light_audio,
                                &light_backlight,
                                // loaded so its one-shot task is scheduled and the baked boot
                                // command gets dispatched; the command tree itself registers
                                // through .light.static regardless
                                &light_cli,
                                &light_core);

static struct touch_device *_touch_main;
// this app's own, not shared through light_ui_demo.h: the shared demo body never touches
// the IMU, only this board's orientation wiring does
static struct imu_device *_imu_main;
// down-edge detector. light_touch reports gestures on release, but a button press should
// land the moment the finger arrives, so this watches the touch state directly rather than
// going through light_touch_take_gesture()
static bool _touch_was_active;

//   a build-time device command. The board has no console to type at, so the command line comes
// from LIGHT_BOOT_COMMAND, baked into the image by CMake and run once at application launch --
// which means the brightness this board starts at is a preset setting rather than a #define
// somebody has to edit this file to change.
static struct light_cli_invocation_result do_cmd_touch169(struct light_cli_invocation *invoke)
{
        // the bare root command does nothing on its own; it exists to hang subcommands off
        return Result_Success;
}
static struct light_cli_invocation_result do_cmd_touch169_backlight(struct light_cli_invocation *invoke)
{
        const uint8_t *value = light_cli_invocation_get_arg_value(invoke, 0);

        if(!value) {
                light_error("backlight: expected a level in 0..%d", LIGHT_BACKLIGHT_LEVEL_MAX);
                return Result_Error;
        }
        //   parsed by hand rather than with strtol: the whole input is a build-time constant of
        // at most four digits, and this reports a bad one precisely rather than silently
        // yielding zero the way atoi() would
        uint32_t level = 0;
        for(const uint8_t *p = value; *p; p++) {
                if(*p < '0' || *p > '9') {
                        light_error("backlight: '%s' is not a number", value);
                        return Result_Error;
                }
                level = (level * 10) + (uint32_t)(*p - '0');
                if(level > LIGHT_BACKLIGHT_LEVEL_MAX) {
                        light_error("backlight: level %s exceeds the maximum of %d",
                                        value, LIGHT_BACKLIGHT_LEVEL_MAX);
                        return Result_Error;
                }
        }
        if(!_backlight_main) {
                light_error("backlight: no backlight device");
                return Result_Error;
        }
        light_info("backlight: setting level to %d from the baked boot command", level);
        light_backlight_set_level(_backlight_main, (uint16_t) level);

        return Result_Success;
}
//   the root name has to match the first token of LIGHT_BOOT_COMMAND: process_command_line()
// treats argv[0] as the root command, exactly as a shell command line does
Light_Command_Define(cmd_touch169, &root_command, "touch169",
                        "commands for the RP2350-Touch-LCD-1.69 demo", do_cmd_touch169, 0, 0);
Light_Command_Define(cmd_touch169_backlight, &cmd_touch169, "backlight",
                        "sets the panel backlight level", do_cmd_touch169_backlight, 1, 1);

void main(int argc, char **argv)
{
        light_framework_init();
        //   (0, NULL), not (argc, argv): this is a bare-metal entry point and the runtime never
        // sets those, so they hold whatever happened to be in the argument registers. Passing
        // that on was harmless while nothing read it, but light_cli parses it -- and garbage
        // argc is indistinguishable from a real command line. Zero means "no command line",
        // which is what lets the baked boot command take over
        light_framework_run(0, NULL);
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

        //   swipe right returns to the previous page. light_ui knows nothing about gestures --
        // the mapping from this board's touch controller onto navigation is an application
        // fact, the same as the tap wiring above and the IMU-to-rotation wiring.
        //
        //   taken unconditionally rather than only when a page is showing, so a gesture is
        // never left queued to fire later. navigate_back() answers false at the top of the
        // tree, which is exactly the case where the swipe should mean nothing
        struct touch_gesture gesture;
        if(light_touch_take_gesture(_touch_main, &gesture)) {
                light_ui_demo_note_activity();
                if(gesture.type == TOUCH_GESTURE_SWIPE_RIGHT && light_ui_navigate_back(_ui))
                        light_debug("swipe: returned to the previous page");
        }
}
