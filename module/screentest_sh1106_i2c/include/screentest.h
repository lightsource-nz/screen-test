#ifndef _SCREENTEST_H
#define _SCREENTEST_H

#include <light.h>
#include <light_platform.h>
#include <light_display.h>
#include <light_display_sh1106.h>

#include <rend.h>

#include <stdint.h>

// TODO implement version fields properly
#define ST_VERSION_STR                  "0.1.0"

#define ST_INFO_STR                     "screen-test v" CF_VERSION_STR

// TODO make display count configurable at runtime
#define ST_DISPLAY_COUNT                1

#define ST_DISPLAY_0_PORT_ID            PORT_I2C_0
#define ST_DISPLAY_0_PIN_SCL            5
#define ST_DISPLAY_0_PIN_SDA            4
// this board breaks out only SCL/SDA/power -- no hardware RESET line
#define ST_DISPLAY_0_PIN_RESET          LIGHT_DISPLAY_IOPORT_PIN_NONE
// common default 7-bit I2C address for this controller family (SA0/D-C# pin tied low) --
// change to 0x3D if the module's address-select pin is tied high instead
#define ST_DISPLAY_0_I2C_ADDR           0x3C

#endif
