#ifndef _LIGHT_DISPLAY_IOPORT_H
#define _LIGHT_DISPLAY_IOPORT_H

#include <light.h>
#include <stdint.h>

#define PORT_I2C_0                      0
#define PORT_I2C_1                      1
#define PORT_I2C_2                      2

#define PORT_SPI_0                      3
#define PORT_SPI_1                      4
#define PORT_SPI_2                      5

#define IO_I2C                          0
#define IO_SPI_4P                       1
#define IO_SPI_3P                       2

struct i2c_state {
        uint8_t pin_scl;
        uint8_t pin_sda;
};
struct spi_state {
        uint8_t pin_sck;
        uint8_t pin_cs;
        uint8_t pin_dc;
        uint8_t pin_mosi;
};
struct io_context {
        uint8_t io_type;
        uint8_t port_id;
        union {
                struct i2c_state i2c;
                struct spi_state spi;
        } io;
        uint8_t pin_reset;
};

extern struct io_context *light_display_ioport_setup_io_i2c(
                        uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs, uint8_t pin_scl, uint8_t pin_sda);
extern struct io_context *light_display_ioport_setup_io_spi_4p(
                        uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs, uint8_t pin_dc, uint8_t pin_sck, uint8_t pin_mosi);
extern struct io_context *light_display_ioport_setup_io_spi_3p(
                        uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs, uint8_t pin_sck, uint8_t pin_mosi);
// I/O functions are all blocking, for now
extern void light_display_ioport_send_command_byte(struct io_context *io, uint8_t cmd);
extern void light_display_ioport_send_data_byte(struct io_context *io, uint8_t data);
extern void light_display_ioport_signal_reset(struct io_context *io);

#endif
