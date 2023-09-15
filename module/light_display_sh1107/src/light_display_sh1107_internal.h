#ifndef _LIGHT_DISPLAY_SH1107_INTERNAL_H
#define _LIGHT_DISPLAY_SH1107_INTERNAL_H

// internal function signatures for portable platform I/O interface
extern void _platform_sh1107_i2c_port_init(struct sh1107_io_context *io);
extern void _platform_sh1107_spi3_port_init(struct sh1107_io_context *io);
extern void _platform_sh1107_spi4_port_init(struct sh1107_io_context *io);

extern void _platform_sh1107_signal_reset(struct sh1107_io_context *io);

extern void _platform_sh1107_i2c_send_command_byte(struct sh1107_io_context *io, uint8_t cmd);
extern void _platform_sh1107_i2c_send_data_byte(struct sh1107_io_context *io, uint8_t data);
extern void _platform_sh1107_spi3_send_command_byte(struct sh1107_io_context *io, uint8_t cmd);
extern void _platform_sh1107_spi3_send_data_byte(struct sh1107_io_context *io, uint8_t data);
extern void _platform_sh1107_spi4_send_command_byte(struct sh1107_io_context *io, uint8_t cmd);
extern void _platform_sh1107_spi4_send_data_byte(struct sh1107_io_context *io, uint8_t data);
#endif
