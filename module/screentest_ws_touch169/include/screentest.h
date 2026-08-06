#ifndef _SCREENTEST_H
#define _SCREENTEST_H

#include <light.h>
#include <light_display.h>

#include <stdint.h>

// TODO implement version fields properly
#define ST_VERSION_STR                  "0.1.0"

#define ST_INFO_STR                     "screen-test v" CF_VERSION_STR

// no display driver wired up yet -- this app only exists to confirm the board itself
// boots and the main loop runs; screentest_common's app.c tolerates zero displays fine,
// since rend renders into an in-memory buffer regardless of whether anything consumes it
#define ST_DISPLAY_COUNT                0

extern struct display_device *_display[ST_DISPLAY_COUNT];

extern void __screentest_hardware_init();

#endif
