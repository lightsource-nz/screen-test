#ifndef _SCREENTEST_PO13_H
#define _SCREENTEST_PO13_H

#include <screentest.h>
#include <light_display_po13.h>

// TODO implement version fields properly
#define ST_VERSION_STR                  "0.1.0"

#define ST_INFO_STR                     "screen-test v" CF_VERSION_STR

// TODO make display count configurable at runtime
#define ST_DISPLAY_COUNT                2

#define ST_DISPLAY_1_PIN_CS             17
#define ST_DISPLAY_1_PIN_DC             16
#define ST_DISPLAY_1_PIN_SCK            18
#define ST_DISPLAY_1_PIN_TX             19
#define ST_DISPLAY_1_PIN_RESET          20

#endif