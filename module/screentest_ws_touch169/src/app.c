#include <screentest.h>
#include <screentest_hw_ws_touch169.h>

void __screentest_hardware_init();

// app: screentest_ws_touch169
// the shared animated-circle demo on the RP2350-Touch-LCD-1.69, driven around the canvas by
// touch swipes and by tilting the board. the board's wiring lives in
// screentest_hw_ws_touch169, so the UI demo app can share it

void __screentest_hardware_init()
{
        _display[0] = screentest_hw_ws_touch169_display();
        _touch_main = screentest_hw_ws_touch169_touch();
        _imu_main = screentest_hw_ws_touch169_imu();
        // creating it is what lights the panel: the device starts at full brightness and
        // applies that during init. this demo has no idle behaviour and never dims, but it
        // still has to create the backlight, because the pin is no longer driven by the
        // display setup -- light_backlight owns it now
        screentest_hw_ws_touch169_backlight();
}
