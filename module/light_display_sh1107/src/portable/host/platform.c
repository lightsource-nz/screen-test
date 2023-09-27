// portable/host/platform.c
// implementation of IO routines for dev host platform

#include <light_display_sh1107.h>
#include <stdio.h>

#include "../../light_display_sh1107_internal.h"

void _platform_sh1107_i2c_port_init(struct sh1107_io_context *io)
{
        light_debug("i2c port id 0x%x opened", io->port_id);
}
void _platform_sh1107_spi3_port_init(struct sh1107_io_context *io)
{
        light_debug("spi port id 0x%x opened", io->port_id);
}
void _platform_sh1107_spi4_port_init(struct sh1107_io_context *io)
{
        light_debug("spi port id 0x%x opened", io->port_id);
}

void _platform_sh1107_signal_reset(struct sh1107_io_context *io)
{
        light_debug("chip reset signaled on port id 0x%x", io->port_id);
}

void _platform_sh1107_i2c_send_command_byte(struct sh1107_io_context *io, uint8_t cmd)
{
}
void _platform_sh1107_i2c_send_data_byte(struct sh1107_io_context *io, uint8_t data)
{
}
void _platform_sh1107_spi3_send_command_byte(struct sh1107_io_context *io, uint8_t cmd)
{
}
void _platform_sh1107_spi3_send_data_byte(struct sh1107_io_context *io, uint8_t data)
{
}
void _platform_sh1107_spi4_send_command_byte(struct sh1107_io_context *io, uint8_t cmd)
{
}
void _platform_sh1107_spi4_send_data_byte(struct sh1107_io_context *io, uint8_t data)
{
}
