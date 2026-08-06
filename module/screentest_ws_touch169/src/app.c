#include <screentest.h>

void __screentest_hardware_init();

// app: screentest_ws_touch169
// board bring-up milestone only -- no display/touch/IMU/RTC driver wired up yet, just
// confirms the Waveshare RP2350-Touch-LCD-1.69's chip/board plumbing boots and the
// framework's main loop runs

void __screentest_hardware_init()
{
        light_info("board bring-up: no display driver wired up yet","");
}
