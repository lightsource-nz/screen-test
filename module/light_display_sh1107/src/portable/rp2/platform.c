// portable/rp2/platform.c
// implementation of IO routines for RP2 series MCUs

#include <light_display_sh1107.h>

#include "../../light_display_sh1107_internal.h"

#include <pico/time.h>
#include <hardware/i2c.h>
#include <hardware/spi.h>
#include <hardware/gpio.h>

#define I2C_BAUDRATE                    (300 * 1000)
#define SPI_BAUDRATE                    (10000 * 1000)

static i2c_inst_t *_i2c_select(uint8_t port_id)
{
        switch(port_id) {
        case PORT_I2C_0:
                return i2c0;
        case PORT_I2C_1:
                return i2c1;
        case PORT_I2C_2:
                return NULL;
        }

}
static spi_inst_t *_spi_select(uint8_t port_id)
{
        switch(port_id) {
        case PORT_SPI_0:
                return spi0;
        case PORT_SPI_1:
                return spi1;
        case PORT_SPI_2:
                return NULL;
        default:
                return NULL;
        }
}

void _platform_sh1107_i2c_port_init(struct sh1107_io_context *io)
{
        i2c_inst_t *port;
        if(!(port = _i2c_select(io->port_id))) {
                light_warn("failed: port id 0x%x is not a valid i2c port", io->port_id);
                return;
        }
        gpio_set_function(io->io.i2c.pin_scl, GPIO_FUNC_I2C);
        gpio_set_function(io->io.i2c.pin_sda, GPIO_FUNC_I2C);
        gpio_set_function(io->pin_reset, GPIO_FUNC_SIO);
        gpio_set_dir(io->pin_reset, true);
        uint rate = i2c_init(port, I2C_BAUDRATE);
        light_debug("i2c port id 0x%x opened with baud rate %d", io->port_id, rate);
}

void _platform_sh1107_spi3_port_init(struct sh1107_io_context *io)
{
        spi_inst_t *port;
        if(!(port = _spi_select(io->port_id))) {
                light_warn("failed: port id 0x%x is not a valid spi port", io->port_id);
                return;
        }
        gpio_set_function(io->io.spi.pin_sck, GPIO_FUNC_SPI);
        gpio_set_function(io->io.spi.pin_mosi, GPIO_FUNC_SPI);
        gpio_set_function(io->pin_reset, GPIO_FUNC_SIO);
        gpio_set_dir(io->pin_reset, true);
        gpio_set_function(io->io.spi.pin_cs, GPIO_FUNC_SIO);
        gpio_set_dir(io->io.spi.pin_cs, true);
        uint rate = spi_init(port, SPI_BAUDRATE);
        light_debug("spi port id 0x%x opened with baud rate %d", io->port_id, rate);
}
void _platform_sh1107_spi4_port_init(struct sh1107_io_context *io)
{
        spi_inst_t *port;
        if(!(port = _spi_select(io->port_id))) {
                light_warn("failed: port id 0x%x is not a valid spi port", io->port_id);
                return;
        }
        gpio_set_function(io->io.spi.pin_sck, GPIO_FUNC_SPI);
        gpio_set_function(io->io.spi.pin_mosi, GPIO_FUNC_SPI);
        gpio_set_function(io->pin_reset, GPIO_FUNC_SIO);
        gpio_set_dir(io->pin_reset, true);
        gpio_set_function(io->io.spi.pin_cs, GPIO_FUNC_SIO);
        gpio_set_dir(io->io.spi.pin_cs, true);
        gpio_set_function(io->io.spi.pin_dc, GPIO_FUNC_SIO);
        gpio_set_dir(io->io.spi.pin_dc, true);
        uint rate = spi_init(port, SPI_BAUDRATE);
        light_debug("spi port id 0x%x opened with baud rate %d", io->port_id, rate);
}

void _platform_sh1107_signal_reset(struct sh1107_io_context *io)
{
        gpio_put(io->pin_reset, true);
        sleep_ms(100);
        gpio_put(io->pin_reset, false);
        sleep_ms(100);
        gpio_put(io->pin_reset, true);
        sleep_ms(100);
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
        gpio_put(io->io.spi.pin_dc,false);
        gpio_put(io->io.spi.pin_cs,false);

        spi_write_blocking(_spi_select(io->port_id), &cmd, 1);
        gpio_put(io->io.spi.pin_cs,true);
}
void _platform_sh1107_spi4_send_data_byte(struct sh1107_io_context *io, uint8_t data)
{
        gpio_put(io->io.spi.pin_dc,true);
        gpio_put(io->io.spi.pin_cs,false);

        spi_write_blocking(_spi_select(io->port_id), &data, 1);
        gpio_put(io->io.spi.pin_cs,true);
}
