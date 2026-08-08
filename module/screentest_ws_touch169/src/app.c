#include <screentest.h>
#include <screentest_hw_ws_touch169.h>

void __screentest_hardware_init();

// app: screentest_ws_touch169
// the shared animated-circle demo on the RP2350-Touch-LCD-1.69, driven around the canvas by
// touch swipes. the board's wiring lives in screentest_hw_ws_touch169, so the UI demo app
// can share it

void __screentest_hardware_init()
{
        _display[0] = screentest_hw_ws_touch169_display();
        _touch_main = screentest_hw_ws_touch169_touch();
}
