#include <screentest.h>
#include <screentest_hw_po13.h>

void __screentest_hardware_init();

// app: screentest_po13
// the shared animated-circle demo on the two-display Pico-OLED-1.3 rig: the board's own
// panel, plus a second SH1107 of the same geometry on spi0. the wiring itself lives in
// screentest_hw_po13, so the UI demo app can share it

void __screentest_hardware_init()
{
        _display[0] = screentest_hw_po13_display_main();
        _display[1] = screentest_hw_po13_display_sec();
        light_info("display pipeline setup complete","");
}
