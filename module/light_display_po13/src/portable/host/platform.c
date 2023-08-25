// portable/host/platform.c
// implementation of IO routines for dev host platform

#include <light_display_po13.h>
#include <stdio.h>

#include "../../light_display_po13_internal.h"

void _platform_sh1107_i2c_port_init(
                                uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs,
                                uint8_t pin_scl, uint8_t pin_sda)
{
        light_debug("i2c port id 0x%x opened", port_id);
}
void _platform_sh1107_spi3_port_init(
                                uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs,
                                uint8_t pin_sck, uint8_t pin_mosi)
{
        light_debug("spi port id 0x%x opened", port_id);
}
void _platform_sh1107_spi4_port_init(
                                uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs, 
                                uint8_t pin_dc, uint8_t pin_sck, uint8_t pin_mosi)
{
        light_debug("spi port id 0x%x opened", port_id);
}
