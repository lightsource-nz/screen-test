#ifndef _LIGHT_DISPLAY_PO13_H
#define _LIGHT_DISPLAY_PO13_H

#include <light_display.h>
#include <module/mod_light_display_po13.h>

#include <stdint.h>

#define PORT_I2C_0                      0
#define PORT_I2C_1                      1
#define PORT_I2C_2                      2

#define PORT_SPI_0                      3
#define PORT_SPI_1                      4
#define PORT_SPI_2                      5

#define SH1107_IO_I2C                   0
#define SH1107_IO_SPI                   1

#define PO13_WIDTH                      128
#define PO13_HEIGHT                     64
#define PO13_BPP                        1

struct i2c_state {
        uint8_t pin_sck;
        uint8_t pin_sda;
};
struct spi_state {
        uint8_t pin_sclk;
        uint8_t pin_cs;
        uint8_t pin_dc;
        uint8_t pin_miso;
        uint8_t pin_mosi;
};
struct sh1107_io_context {
        uint8_t io_type;
        uint8_t port_id;
        union {
                struct i2c_state i2c;
                struct spi_state spi;
        } io;
};

extern struct display_driver *light_display_driver_sh1107();

extern struct sh1107_io_context *light_display_sh1107_setup_io_i2c(
                        uint8_t port_id, uint8_t pin_sck, uint8_t pin_sda);
extern struct sh1107_io_context *light_display_sh1107_setup_io_spi(
                        uint8_t port_id, uint8_t pin_sclk, uint8_t pin_cs, uint8_t pin_dc, uint8_t pin_miso, uint8_t pin_mosi);

extern struct display_device *light_display_po13_create_device(uint8_t *name, struct sh1107_io_context *io);
extern struct display_device *light_display_sh1107_create_device(
        uint8_t *name, uint16_t width, uint16_t height, uint8_t bpp, struct sh1107_io_context *io);

#endif