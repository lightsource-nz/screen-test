#ifndef _LIGHT_DISPLAY_H
#define _LIGHT_DISPLAY_H

#include <light.h>

#include <stdint.h>

// TODO implement version fields properly
#define IO_CMD_VERSION_STR                  "0.1.0"

#define IO_CMD_INFO_STR                     "io_command v" IO_CMD_VERSION_STR

#define CMD_PIN_SET                         0
#define CMD_PIN_GET                         1
#define CMD_WAIT_MS                         2
#define CMD_SEQUENCE                        3

struct io_command {
        uint8_t type;
};

#define COMMAND(type, data)

#endif