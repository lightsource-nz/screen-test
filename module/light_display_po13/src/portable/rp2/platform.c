// portable/rp2/platform.c
// implementation of IO routines for RP2 series MCUs

#include <light_display_po13.h>

#include "../../light_display_po13_internal.h"

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

void _platform_sh1107_i2c_port_init(
                                uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs,
                                uint8_t pin_scl, uint8_t pin_sda)
{
        i2c_inst_t *port;
        if(!(port = _i2c_select(port_id))) {
                light_warn("failed: port id 0x%x is not a valid i2c port", port_id);
                return;
        }
        gpio_set_function(pin_scl, GPIO_FUNC_I2C);
        gpio_set_function(pin_sda, GPIO_FUNC_I2C);
        gpio_set_function(pin_reset, GPIO_FUNC_SIO);
        gpio_set_dir(pin_reset, true);
        gpio_set_function(pin_cs, GPIO_FUNC_SIO);
        gpio_set_dir(pin_cs, true);
        uint rate = i2c_init(port, I2C_BAUDRATE);
        light_debug("i2c port id 0x%x opened with baud rate %d", port_id, rate);
}

void _platform_sh1107_spi3_port_init(
                                uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs,
                                uint8_t pin_sck, uint8_t pin_mosi)
{
        spi_inst_t *port;
        if(!(port = _spi_select(port_id))) {
                light_warn("failed: port id 0x%x is not a valid spi port", port_id);
                return;
        }
        gpio_set_function(pin_sck, GPIO_FUNC_SPI);
        gpio_set_function(pin_mosi, GPIO_FUNC_SPI);
        gpio_set_function(pin_reset, GPIO_FUNC_SIO);
        gpio_set_dir(pin_reset, true);
        gpio_set_function(pin_cs, GPIO_FUNC_SIO);
        gpio_set_dir(pin_cs, true);
        uint rate = spi_init(port, SPI_BAUDRATE);
        light_debug("spi port id 0x%x opened with baud rate %d", port_id, rate);
}
void _platform_sh1107_spi4_port_init(
                                uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs, 
                                uint8_t pin_dc, uint8_t pin_sck, uint8_t pin_mosi)
{
        spi_inst_t *port;
        if(!(port = _spi_select(port_id))) {
                light_warn("failed: port id 0x%x is not a valid spi port", port_id);
                return;
        }
        gpio_set_function(pin_sck, GPIO_FUNC_SPI);
        gpio_set_function(pin_mosi, GPIO_FUNC_SPI);
        gpio_set_function(pin_reset, GPIO_FUNC_SIO);
        gpio_set_dir(pin_reset, true);
        gpio_set_function(pin_cs, GPIO_FUNC_SIO);
        gpio_set_dir(pin_cs, true);
        gpio_set_function(pin_dc, GPIO_FUNC_SIO);
        gpio_set_dir(pin_dc, true);
        uint rate = spi_init(port, SPI_BAUDRATE);
        light_debug("spi port id 0x%x opened with baud rate %d", port_id, rate);
}
