#ifndef _LIGHT_DISPLAY_PO13_INTERNAL_H
#define _LIGHT_DISPLAY_PO13_INTERNAL_H

// internal function signatures for portable platform I/O interface
// TODO check pin assignment for i2c port, I think pin_cs might not be used
extern void _platform_sh1107_i2c_port_init(
                                uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs,
                                uint8_t pin_scl, uint8_t pin_sda);
extern void _platform_sh1107_spi3_port_init(
                                uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs,
                                uint8_t pin_sck, uint8_t pin_mosi);
extern void _platform_sh1107_spi4_port_init(
                                uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs, 
                                uint8_t pin_dc, uint8_t pin_sck, uint8_t pin_mosi);

#endif
