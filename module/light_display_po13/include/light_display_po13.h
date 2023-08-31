#ifndef _LIGHT_DISPLAY_PO13_H
#define _LIGHT_DISPLAY_PO13_H

#include <light_display.h>
#include <module/mod_light_display_po13.h>

#include <stdint.h>

#define SH1107_ADDRMODE_PAGE            0
#define SH1107_ADDRMODE_VERTICAL        1

#define SH1107_SCAN_DIR_DOWN            0
#define SH1107_SCAN_DIR_UP              1

// SMPS switching frequency = (~500kHz * factor)
#define SH1107_POWER_FACTOR_0_6         0       // reset value: 0.6
#define SH1107_POWER_FACTOR_0_7         1
#define SH1107_POWER_FACTOR_0_8         2
#define SH1107_POWER_FACTOR_0_9         3
#define SH1107_POWER_FACTOR_1_0         4
#define SH1107_POWER_FACTOR_1_1         5
#define SH1107_POWER_FACTOR_1_2         6
#define SH1107_POWER_FACTOR_1_3         7

// display clock oscillator frequency = (fOSC * factor)
#define SH1107_FOSC_FACTOR_0_75         0x0
#define SH1107_FOSC_FACTOR_0_8          0x1
#define SH1107_FOSC_FACTOR_0_85         0x2
#define SH1107_FOSC_FACTOR_0_9          0x3
#define SH1107_FOSC_FACTOR_0_95         0x4
#define SH1107_FOSC_FACTOR_1_0          0x5     // reset value: 1.0
#define SH1107_FOSC_FACTOR_1_05         0x6
#define SH1107_FOSC_FACTOR_1_1          0x7
#define SH1107_FOSC_FACTOR_1_15         0x8
#define SH1107_FOSC_FACTOR_1_2          0x9
#define SH1107_FOSC_FACTOR_1_25         0xA
#define SH1107_FOSC_FACTOR_1_3          0xB
#define SH1107_FOSC_FACTOR_1_35         0xC
#define SH1107_FOSC_FACTOR_1_4          0xD
#define SH1107_FOSC_FACTOR_1_45         0xE
#define SH1107_FOSC_FACTOR_1_5          0xF

#define SH1107_CMD_SET_COL_ADDR_LOW     0x00
#define SH1107_CMD_SET_COL_ADDR_HIGH    0x10
#define SH1107_CMD_SET_ADDRMODE         0x20
#define SH1107_CMD_SET_CONTRAST         0x81
#define SH1107_CMD_SET_SEG_REMAP        0xA0
#define SH1107_CMD_SET_MUX_RATIO        0xA8
#define SH1107_CMD_SET_FORCE_ON         0xA4
#define SH1107_CMD_SET_REVERSE          0xA6
#define SH1107_CMD_SET_DISPLAY_OFFSET   0xD3
#define SH1107_CMD_SET_POWER_MODE       0xAD
#define SH1107_CMD_SET_DISPLAY_ON       0xAE
#define SH1107_CMD_SET_PAGE_ADDR        0xB0
#define SH1107_CMD_SET_SCAN_DIR         0xC0
#define SH1107_CMD_SET_DISPLAY_CLK      0xD5
#define SH1107_CMD_SET_CHARGE_PERIODS   0xD9
#define SH1107_CMD_SET_VCOMH            0xDB
#define SH1107_CMD_SET_DISPLAY_START    0xDC
#define SH1107_CMD_RMW_BEGIN            0xE0
#define SH1107_CMD_NOP                  0xE3
#define SH1107_CMD_RMW_END              0xEE

#define PORT_I2C_0                      0
#define PORT_I2C_1                      1
#define PORT_I2C_2                      2

#define PORT_SPI_0                      3
#define PORT_SPI_1                      4
#define PORT_SPI_2                      5

#define SH1107_IO_I2C                   0
#define SH1107_IO_SPI_4P                1
#define SH1107_IO_SPI_3P                2

#define PO13_WIDTH                      128
#define PO13_HEIGHT                     64
#define PO13_BPP                        1

#define PO13_PIN_RESET                  12
#define PO13_PIN_CS                     9
#define PO13_PIN_DC                     8

#define PO13_PIN_I2C_SCL                7
#define PO13_PIN_I2C_SDA                6

#define PO13_PIN_SPI_SCK                10
#define PO13_PIN_SPI_MOSI               11

struct i2c_state {
        uint8_t pin_scl;
        uint8_t pin_sda;
};
struct spi_state {
        uint8_t pin_sck;
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
        uint8_t pin_reset;
};

extern struct display_driver *light_display_driver_sh1107();

extern struct sh1107_io_context *light_display_po13_setup_io_i2c(uint8_t port_id);
extern struct sh1107_io_context *light_display_po13_setup_io_spi_4p(uint8_t port_id);
extern struct sh1107_io_context *light_display_po13_setup_io_spi_3p(uint8_t port_id);

extern struct sh1107_io_context *light_display_sh1107_setup_io_i2c(
                        uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs, uint8_t pin_scl, uint8_t pin_sda);
extern struct sh1107_io_context *light_display_sh1107_setup_io_spi_4p(
                        uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs, uint8_t pin_dc, uint8_t pin_sck, uint8_t pin_mosi);
extern struct sh1107_io_context *light_display_sh1107_setup_io_spi_3p(
                        uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs, uint8_t pin_sck, uint8_t pin_mosi);
// I/O functions are all blocking, for now
extern void light_display_sh1107_io_send_command_byte(struct sh1107_io_context *io, uint8_t cmd);
extern void light_display_sh1107_io_send_data_byte(struct sh1107_io_context *io, uint8_t data);
extern void light_display_sh1107_io_signal_reset(struct sh1107_io_context *io);

extern struct display_device *light_display_po13_create_device(uint8_t *name, struct sh1107_io_context *io);
extern struct display_device *light_display_sh1107_create_device(
        uint8_t *name, uint16_t width, uint16_t height, uint8_t bpp, struct sh1107_io_context *io);
extern void light_display_sh1107_reset_device(struct display_device *dev);
extern void light_display_sh1107_chip_setup(struct display_device *dev);
extern void light_display_sh1107_command_set_column_addr(struct display_device *dev, uint8_t addr);
extern void light_display_sh1107_command_set_addrmode(struct display_device *dev, uint8_t mode);
extern void light_display_sh1107_command_set_contrast(struct display_device *dev, uint8_t level);
extern void light_display_sh1107_command_set_segment_remap(struct display_device *dev, bool enable);
extern void light_display_sh1107_command_set_multiplex_ratio(struct display_device *dev, uint8_t ratio);
extern void light_display_sh1107_command_set_force_on(struct display_device *dev, bool enable);
extern void light_display_sh1107_command_set_reverse_display(struct display_device *dev, bool enable);
extern void light_display_sh1107_command_set_display_offset(struct display_device *dev, uint8_t data);
extern void light_display_sh1107_command_set_power_mode(struct display_device *dev, bool enable, uint8_t mode);
extern void light_display_sh1107_command_set_display_on(struct display_device *dev, bool enable);
extern void light_display_sh1107_command_set_page_addr(struct display_device *dev, uint8_t addr);
extern void light_display_sh1107_command_set_scan_dir(struct display_device *dev, uint8_t data);
extern void light_display_sh1107_command_set_display_clock(struct display_device *dev, uint8_t div, uint8_t freq);
extern void light_display_sh1107_command_set_charge_periods(struct display_device *dev, uint8_t pre, uint8_t dis);
extern void light_display_sh1107_command_set_vcom_deselect(struct display_device *dev, uint8_t data);
extern void light_display_sh1107_command_set_start_line(struct display_device *dev, uint8_t addr);
extern void light_display_sh1107_command_rmw_begin(struct display_device *dev);
extern void light_display_sh1107_command_rmw_end(struct display_device *dev);
extern void light_display_sh1107_command_no_op(struct display_device *dev);
extern void light_display_sh1107_write_data(struct display_device *dev, uint8_t data);
extern uint16_t light_display_sh1107_y_to_pages(uint16_t y);
extern uint16_t light_display_sh1107_x_to_columns(uint16_t x);
#endif
