#include <light_display_po13.h>

#include "light_display_po13_internal.h"

struct sh1107_state {
        struct sh1107_io_context *io_ctx;
        uint8_t addrmode;
        uint16_t n_pages;
        uint16_t n_columns;
};

static struct display_driver_context *_sh1107_spawn_context();
static void _sh1107_init(struct display_device *dev);
static void _sh1107_reset(struct display_device *dev);
static void _sh1107_clear(struct display_device *dev, uint8_t value);
static void _sh1107_update(struct display_device *dev);

static struct display_driver _driver_sh1107 = {
        .name = "display.driver:sh1107",
        .spawn_context = _sh1107_spawn_context,
        .init_device = _sh1107_init,
        .reset = _sh1107_reset,
        .clear = _sh1107_clear,
        .update = _sh1107_update
};

struct display_driver *light_display_driver_sh1107()
{
        return &_driver_sh1107;
}

static struct display_driver_context *_sh1107_spawn_context()
{
        struct display_driver_context *ctx = light_alloc(sizeof(struct display_driver_context));
        ctx->driver = light_display_driver_sh1107();
        ctx->state = light_alloc(sizeof(struct sh1107_state));
        return ctx;
}

static void _sh1107_init(struct display_device *dev)
{
        // ASSERT dev->driver_ctx->driver == light_display_driver_sh1007()
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        state->n_pages = light_display_sh1107_y_to_pages(dev->height);
        state->n_columns = light_display_sh1107_x_to_columns(dev->width);
        light_display_sh1107_chip_setup(dev);

}
static void _sh1107_reset(struct display_device *dev)
{
        // ASSERT dev->driver_ctx->driver == light_display_driver_sh1007()
        light_display_sh1107_reset_device(dev);
}
static void _sh1107_clear(struct display_device *dev, uint8_t value)                                                                                                                                                                                                                                                                                                                  
{
        // ASSERT dev->driver_ctx->driver === &_driver_sh1107
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_command_set_page_addr(dev, 0);
        for(uint16_t column = 0; column < state->n_columns; column++) {
                light_display_sh1107_command_set_column_addr(dev, column);
                for(uint16_t page = 0; page < state->n_columns; page++) {
                        light_display_sh1107_write_data(dev, value);
                }
        }
}
static void _sh1107_update(struct display_device *dev)
{
        // ASSERT dev->driver_ctx->driver === &_driver_sh1107
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;

        // write out each column page by page, starting with column 0.
        // page address increments automatically, and wraps at the end of each column
        light_display_sh1107_command_set_page_addr(dev, 0);
        for(uint16_t column = 0; column < state->n_columns; column++) {
                light_display_sh1107_command_set_column_addr(dev, column);
                for(uint16_t page = 0; page < state->n_columns; page++) {
                        light_display_sh1107_write_data(dev, dev->render_ctx->buffer[column * state->n_pages + page]);
                }
        }

}

// I/O port abstractions:
//      for now we just hard-code mappings for RP2040 targets.
//      TODO add a decoupling layer to support other targets

struct sh1107_io_context *light_display_po13_setup_io_i2c(uint8_t port_id)
{
        return light_display_sh1107_setup_io_i2c(
                port_id, PO13_PIN_RESET, PO13_PIN_CS, PO13_PIN_I2C_SCL, PO13_PIN_I2C_SDA);
}
struct sh1107_io_context *light_display_po13_setup_io_spi_4p(uint8_t port_id)
{
        return light_display_sh1107_setup_io_spi_4p(
                port_id, PO13_PIN_RESET, PO13_PIN_CS, PO13_PIN_DC,
                                PO13_PIN_SPI_SCK, PO13_PIN_SPI_MOSI);
}
struct sh1107_io_context *light_display_po13_setup_io_spi_3p(uint8_t port_id)
{
        return light_display_sh1107_setup_io_spi_3p(
                port_id, PO13_PIN_RESET, PO13_PIN_CS, PO13_PIN_SPI_SCK, PO13_PIN_SPI_MOSI);
}

struct sh1107_io_context *light_display_sh1107_setup_io_i2c(
                uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs, uint8_t pin_scl, uint8_t pin_sda)
{
        // ASSERT  _port_is_i2c(port_id)
        struct sh1107_io_context *io = light_alloc(sizeof(*io));
        io->io_type = SH1107_IO_I2C;
        io->port_id = port_id;
        io->io.i2c.pin_scl = pin_scl;
        io->io.i2c.pin_sda = pin_sda;

        _platform_sh1107_i2c_port_init(io);
        return io;
}
struct sh1107_io_context *light_display_sh1107_setup_io_spi_4p(
                        uint8_t port_id, uint8_t pin_reset, uint8_t pin_cs,
                        uint8_t pin_dc, uint8_t pin_sck, uint8_t pin_mosi)
{
        // ASSERT  _port_is_spi(port_id)
        struct sh1107_io_context *io = light_alloc(sizeof(*io));
        io->io_type = SH1107_IO_SPI_4P;
        io->port_id = port_id;
        io->io.spi.pin_sck = pin_sck;
        io->io.spi.pin_cs = pin_cs;
        io->io.spi.pin_dc = pin_dc;
        io->io.spi.pin_mosi = pin_mosi;
        _platform_sh1107_spi4_port_init(io);
        return io;
}
struct sh1107_io_context *light_display_sh1107_setup_io_spi_3p(
                                uint8_t port_id, uint8_t pin_reset,
                                uint8_t pin_cs, uint8_t pin_sck, uint8_t pin_mosi)
{
        // ASSERT  _port_is_spi(port_id)
        struct sh1107_io_context *io = light_alloc(sizeof(*io));
        io->io_type = SH1107_IO_SPI_3P;
        io->port_id = port_id;
        io->io.spi.pin_sck = pin_sck;
        io->io.spi.pin_cs = pin_cs;
        io->io.spi.pin_mosi = pin_mosi;
        _platform_sh1107_spi3_port_init(io);
        return io;
}

void light_display_sh1107_io_send_command_byte(struct sh1107_io_context *io, uint8_t cmd)
{
        switch(io->io_type) {
        case SH1107_IO_I2C:
                _platform_sh1107_i2c_send_command_byte(io, cmd);
                break;
        case SH1107_IO_SPI_3P:
                _platform_sh1107_spi3_send_command_byte(io, cmd);
                break;
        case SH1107_IO_SPI_4P:
                _platform_sh1107_spi4_send_command_byte(io, cmd);
                break;
        default:
                light_warn("invalid I/O context type code: 0x%x", io->io_type);
        }
}
void light_display_sh1107_io_send_data_byte(struct sh1107_io_context *io, uint8_t data)
{
        switch(io->io_type) {
        case SH1107_IO_I2C:
                _platform_sh1107_i2c_send_data_byte(io, data);
                break;
        case SH1107_IO_SPI_3P:
                _platform_sh1107_spi3_send_data_byte(io, data);
                break;
        case SH1107_IO_SPI_4P:
                _platform_sh1107_spi4_send_data_byte(io, data);
                break;
        default:
                light_warn("invalid I/O context type code: 0x%x", io->io_type);
        }
}
void light_display_sh1107_io_signal_reset(struct sh1107_io_context *io)
{
        _platform_sh1107_signal_reset(io);
}
/*
void light_display_sh1107_io_chip_setup(struct sh1107_io_context *io)
{
        light_display_sh1107_io_signal_reset(io);
        light_display_sh1107_io_send_command_byte(io, 0xAE);    // display OFF
        light_display_sh1107_io_send_command_byte(io, 0x00);    // 
        light_display_sh1107_io_send_command_byte(io, 0x10);    // set column address (0x0)
        light_display_sh1107_io_send_command_byte(io, 0x80);    // set page address (0x0)
        light_display_sh1107_io_send_command_byte(io, 0xDC);    // 
        light_display_sh1107_io_send_command_byte(io, 0x00);    // set display start line (0)
        light_display_sh1107_io_send_command_byte(io, 0x81);    // 
        light_display_sh1107_io_send_command_byte(io, 0x6F);    // set contrast level (128)
        light_display_sh1107_io_send_command_byte(io, 0x21);    // RAM addressing mode (vertical)
        light_display_sh1107_io_send_command_byte(io, 0xA0);    // set segment remap OFF
        light_display_sh1107_io_send_command_byte(io, 0xC0);    // set common scan direction (down)
        light_display_sh1107_io_send_command_byte(io, 0xA4);    // set force all pixels (disable)
        light_display_sh1107_io_send_command_byte(io, 0xA6);    // set reverse mode OFF
        light_display_sh1107_io_send_command_byte(io, 0xA8);    // 
        light_display_sh1107_io_send_command_byte(io, 0x3F);    // set multiplex ratio (1:64)
        light_display_sh1107_io_send_command_byte(io, 0xD3);    // 
        light_display_sh1107_io_send_command_byte(io, 0x60);    // set display offset (48)
        light_display_sh1107_io_send_command_byte(io, 0xD5);    // 
        light_display_sh1107_io_send_command_byte(io, 0x41);    // set oscillator freq ([f-5%]/2)
        light_display_sh1107_io_send_command_byte(io, 0xD9);    // 
        light_display_sh1107_io_send_command_byte(io, 0x22);    // set pre-charge (2), dis-charge (2)
        light_display_sh1107_io_send_command_byte(io, 0xDB);    // 
        light_display_sh1107_io_send_command_byte(io, 0x35);    // set VcomH (0.770 x Vref)
        light_display_sh1107_io_send_command_byte(io, 0xAD);    // 
        light_display_sh1107_io_send_command_byte(io, 0x8A);    // set built-in DC-DC OFF
}
*/
void light_display_sh1107_reset_device(struct display_device *dev)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_signal_reset(state->io_ctx);
}
void light_display_sh1107_chip_setup(struct display_device *dev)
{
        light_display_sh1107_reset_device(dev);
        light_display_sh1107_command_set_display_on(dev, false);        // display OFF
        light_display_sh1107_command_set_column_addr(dev, 0x0);         // set column address (0x0)
        light_display_sh1107_command_set_page_addr(dev, 0x0);           // set page address (0x0)
        light_display_sh1107_command_set_start_line(dev, 0x0);          // set display start line (0)
        light_display_sh1107_command_set_contrast(dev, 128);            // set contrast level (128)
        light_display_sh1107_command_set_addrmode(dev, SH1107_ADDRMODE_VERTICAL);       // RAM addressing mode (vertical)
        light_display_sh1107_command_set_segment_remap(dev, false);     // set segment remap OFF
        light_display_sh1107_command_set_scan_dir(dev, SH1107_SCAN_DIR_DOWN);   // set common scan direction (down)
        light_display_sh1107_command_set_force_on(dev, false);          // set force all pixels (disable)
        light_display_sh1107_command_set_reverse_display(dev, false);   // set reverse mode OFF
        light_display_sh1107_command_set_multiplex_ratio(dev, 128);     // set multiplex ratio (1:64)
        light_display_sh1107_command_set_display_offset(dev, 48);       // set display offset (48)
        light_display_sh1107_command_set_display_clock(dev, 4, 1);      // set oscillator freq ([f-5%]/2)
        light_display_sh1107_command_set_charge_periods(dev, 2, 2);     // set pre-charge (2), dis-charge (2)
        light_display_sh1107_command_set_vcom_deselect(dev, 0x35);      // set VcomH (0.770 x Vref)
        light_display_sh1107_command_set_power_mode(dev, false, 0);     // set built-in DC-DC OFF
}
struct display_device *light_display_po13_create_device(uint8_t *name, struct sh1107_io_context *io)
{
        return light_display_sh1107_create_device(name, PO13_WIDTH, PO13_HEIGHT, PO13_BPP, io);
}
struct display_device *light_display_sh1107_create_device(uint8_t *name, uint16_t width, uint16_t height, uint8_t bpp, struct sh1107_io_context *io)
{
        // TODO validate *io
        struct display_device *dev = light_display_create_device(light_display_driver_sh1107(), width, height, bpp);
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        state->io_ctx = io;
        light_display_add_device(dev, name);

        return dev;    
}

void light_display_sh1107_command_set_column_addr(struct display_device *dev, uint8_t column)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_COL_ADDR_LOW + (column & 0x0F));
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_COL_ADDR_HIGH + (column >> 4));
}
void light_display_sh1107_command_set_addrmode(struct display_device *dev, uint8_t mode)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_ADDRMODE + mode);
}
void light_display_sh1107_command_set_contrast(struct display_device *dev, uint8_t level)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_CONTRAST);
        light_display_sh1107_io_send_command_byte(state->io_ctx, level);
}
void light_display_sh1107_command_set_segment_remap(struct display_device *dev, bool enable)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_SEG_REMAP + enable);
}
extern void light_display_sh1107_command_set_multiplex_ratio(struct display_device *dev, uint8_t ratio)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_MUX_RATIO);
        light_display_sh1107_io_send_command_byte(state->io_ctx, ratio);
}
void light_display_sh1107_command_set_force_on(struct display_device *dev, bool enable)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_FORCE_ON + enable);
}
void light_display_sh1107_command_set_reverse_display(struct display_device *dev, bool enable)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_REVERSE + enable);
}
void light_display_sh1107_command_set_display_offset(struct display_device *dev, uint8_t data)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_DISPLAY_OFFSET);
        light_display_sh1107_io_send_command_byte(state->io_ctx, data);
}
void light_display_sh1107_command_set_power_mode(struct display_device *dev, bool enable, uint8_t mode)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_POWER_MODE);
        light_display_sh1107_io_send_command_byte(state->io_ctx, enable + (mode << 1));
}
void light_display_sh1107_command_set_display_on(struct display_device *dev, bool enable)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_DISPLAY_ON + enable);
}
void light_display_sh1107_command_set_page_addr(struct display_device *dev, uint8_t addr)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_PAGE_ADDR + addr);
}
void light_display_sh1107_command_set_scan_dir(struct display_device *dev, uint8_t data)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_REVERSE + data);
}
void light_display_sh1107_command_set_display_clock(struct display_device *dev, uint8_t div, uint8_t freq)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_DISPLAY_CLK);
        light_display_sh1107_io_send_command_byte(state->io_ctx, (div & 0x0F) + (freq & 0x0F) << 4);
}
void light_display_sh1107_command_set_charge_periods(struct display_device *dev, uint8_t pre, uint8_t dis)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_CHARGE_PERIODS);
        light_display_sh1107_io_send_command_byte(state->io_ctx, (pre & 0x0F) + (dis & 0x0F) << 4);
}
void light_display_sh1107_command_set_vcom_deselect(struct display_device *dev, uint8_t data)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_VCOMH);
        light_display_sh1107_io_send_command_byte(state->io_ctx, data);
}
void light_display_sh1107_command_set_start_line(struct display_device *dev, uint8_t addr)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_SET_DISPLAY_START);
        light_display_sh1107_io_send_command_byte(state->io_ctx, addr);
}
void light_display_sh1107_command_rmw_begin(struct display_device *dev)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_RMW_BEGIN);
}
void light_display_sh1107_command_rmw_end(struct display_device *dev)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_RMW_END);
}
void light_display_sh1107_command_no_op(struct display_device *dev)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        light_display_sh1107_io_send_command_byte(state->io_ctx, SH1107_CMD_NOP);
}
void light_display_sh1107_write_data(struct display_device *dev, uint8_t data)
{
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        switch(state->io_ctx->io_type) {
                case SH1107_IO_I2C:
                _platform_sh1107_i2c_send_data_byte(state->io_ctx, data);
                break;
                case SH1107_IO_SPI_3P:
                _platform_sh1107_spi3_send_data_byte(state->io_ctx, data);
                break;
                case SH1107_IO_SPI_4P:
                _platform_sh1107_spi4_send_data_byte(state->io_ctx, data);
                break;
        }
}
uint16_t light_display_sh1107_y_to_pages(uint16_t y)
{
        return (y / 8) + ((y % 8)? 1 : 0);
}
uint16_t light_display_sh1107_x_to_columns(uint16_t x)
{
        return x;
}
