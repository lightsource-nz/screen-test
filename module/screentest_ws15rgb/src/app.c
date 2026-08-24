#include <screentest.h>
#include <light_ui_hw_ws15rgb.h>

void __screentest_hardware_init();

// app: screentest_ws15rgb
// the shared animated-circle demo on the Waveshare 1.5inch RGB OLED Module. the bring-up
// binary for that panel: no input, no second device, nothing to configure -- if the circle
// grows and shrinks cleanly to all four edges then the SSD1351 driver, the window
// addressing and the SPI wiring are all good, and anything built on top of them starts from
// a known place.
//
// the wiring lives in light_ui_hw_ws15rgb so the UI demo builds against the same rig, the
// same arrangement screentest_po13 and screentest_ws_touch169 have

void __screentest_hardware_init()
{
        // logs its own "display pipeline setup complete" -- no second line here
        _display[0] = light_ui_hw_ws15rgb_display();
}
