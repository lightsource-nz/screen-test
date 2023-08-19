#include <light_display_po13.h>

#include "light_display_po13_internal.h"

struct sh1107_state {
        struct sh1107_io_context *io_ctx;
};

static struct display_driver_context *_sh1107_spawn_context();
static void _sh1107_clear(struct display_device *dev, uint16_t value);
static void _sh1107_update(struct display_device *dev);

static struct display_driver _driver_sh1107 = {
        .spawn_context = _sh1107_spawn_context,
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
        ctx->state = light_alloc(sizeof(struct sh1107_state));
        return ctx;
}

static void _sh1107_clear(struct display_device *dev, uint16_t value)                                                                                                                                                                                                                                                                                                                  
{
        // ASSERT dev->driver_ctx->driver === &_driver_sh1107
        
}
static void _sh1107_update(struct display_device *dev)
{
        // ASSERT dev->driver_ctx->driver === &_driver_sh1107

}

// I/O port abstractions:
//      for now we just hard-code mappings for RP2040 targets.
//      TODO add a decoupling layer to support other targets
struct sh1107_io_context *light_display_sh1107_setup_io_i2c(
                uint8_t port_id, uint8_t pin_sck, uint8_t pin_sda)
{
        // ASSERT  _port_is_i2c(port_id)
        struct sh1107_io_context *io = light_alloc(sizeof(*io));
        io->io_type = SH1107_IO_I2C;
        io->port_id = port_id;
        io->io.i2c.pin_sck = pin_sck;
        io->io.i2c.pin_sda = pin_sda;
        return io;
}
struct sh1107_io_context *light_display_sh1107_setup_io_spi(
                uint8_t port_id, uint8_t pin_sclk, uint8_t pin_cs, uint8_t pin_dc, uint8_t pin_miso, uint8_t pin_mosi)
{
        // ASSERT  _port_is_spi(port_id)
        struct sh1107_io_context *io = light_alloc(sizeof(*io));
        io->io_type = SH1107_IO_SPI;
        io->port_id = port_id;
        io->io.spi.pin_sclk = pin_sclk;
        io->io.spi.pin_cs = pin_cs;
        io->io.spi.pin_dc = pin_dc;
        io->io.spi.pin_miso = pin_miso;
        io->io.spi.pin_mosi = pin_mosi;
        return io;
}

struct display_device *light_display_po13_create_device(uint8_t *name, struct sh1107_io_context *io)
{
        return light_display_sh1107_create_device(name, PO13_WIDTH, PO13_HEIGHT, PO13_BPP, io);
}
struct display_device *light_display_sh1107_create_device(uint8_t *name, uint16_t width, uint16_t height, uint8_t bpp, struct sh1107_io_context *io)
{
        // TODO validate *io
        struct display_device *dev = light_display_create_device(light_display_driver_sh1107(), name, width, height, bpp);
        struct sh1107_state *state = (struct sh1107_state *) dev->driver_ctx->state;
        state->io_ctx = io;

        return dev;
}
