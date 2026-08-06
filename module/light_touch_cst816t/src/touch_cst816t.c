#include <light_touch_cst816t.h>

#include "light_touch_cst816t_internal.h"

#if(LIGHT_SYSTEM == SYSTEM_PICO_SDK)
#include <hardware/gpio.h>
#endif

struct cst816t_state {
        struct io_context *io_ctx;
        // TP_INT -- read directly as a GPIO, not part of light_display_ioport's
        // io_context (see light_touch_cst816t.h)
        uint8_t pin_int;
        // tracks the previous sample's touch_active so poll() can log on the down-edge
        // only, not every tick a touch stays held -- light_alloc() isn't zeroed, so this
        // must be set explicitly in _spawn_context(), same as every other driver state
        // field in this codebase
        bool was_active;
};

static struct touch_driver_context *_cst816t_spawn_context();
static void _cst816t_init(struct touch_device *dev);
static void _cst816t_reset(struct touch_device *dev);
static bool _cst816t_poll(struct touch_device *dev);

static struct touch_driver _driver_cst816t = {
        .name = "touch.driver:cst816t",
        .spawn_context = _cst816t_spawn_context,
        .init_device = _cst816t_init,
        .reset = _cst816t_reset,
        .poll = _cst816t_poll
};

struct touch_driver *light_touch_driver_cst816t()
{
        return &_driver_cst816t;
}

static struct touch_driver_context *_cst816t_spawn_context()
{
        struct touch_driver_context *ctx = light_alloc(sizeof(struct touch_driver_context));
        ctx->driver = light_touch_driver_cst816t();
        ctx->state = light_alloc(sizeof(struct cst816t_state));
        ((struct cst816t_state *) ctx->state)->was_active = false;
        return ctx;
}

static void _cst816t_gpio_int_setup(uint8_t pin_int)
{
#if(LIGHT_SYSTEM == SYSTEM_PICO_SDK)
        gpio_init(pin_int);
        gpio_set_dir(pin_int, false);
        gpio_pull_up(pin_int);
#endif
}
// active-low: the controller pulls this line low when it has new touch data ready, and
// only responds to I2C reads for a short window after doing so -- this is a plain GPIO
// level poll, not a true hardware IRQ vector; simplest correct first step, matching this
// board's existing "poll a GPIO level from a periodic task" pattern (see the ST7789
// driver's async DMA completion poll)
static bool _cst816t_int_asserted(uint8_t pin_int)
{
#if(LIGHT_SYSTEM == SYSTEM_PICO_SDK)
        return !gpio_get(pin_int);
#else
        return false;
#endif
}

static void _cst816t_init(struct touch_device *dev)
{
        struct cst816t_state *state = (struct cst816t_state *) dev->driver_ctx->state;
        _cst816t_gpio_int_setup(state->pin_int);
        _cst816t_reset(dev);

        uint8_t chip_id = 0;
        // log-and-continue, not hard-fail: the register map is cross-referenced from two
        // open-source drivers, not a primary datasheet -- if this turns out wrong on real
        // hardware, a mismatched chip ID shouldn't prevent poll() from still being tried
        if(!light_display_ioport_read_register(state->io_ctx, CST816T_REG_CHIP_ID, &chip_id, 1)) {
                light_warn("failed to read chip ID for device '%s'", dev->header.id);
        } else if(chip_id != CST816T_CHIP_ID) {
                light_warn("unexpected chip ID for device '%s': got 0x%x, expected 0x%x",
                                dev->header.id, chip_id, CST816T_CHIP_ID);
        } else {
                light_info("cst816t chip ID confirmed for device '%s': 0x%x", dev->header.id, chip_id);
        }
}
static void _cst816t_reset(struct touch_device *dev)
{
        struct cst816t_state *state = (struct cst816t_state *) dev->driver_ctx->state;
        light_display_ioport_signal_reset(state->io_ctx);
}
static bool _cst816t_poll(struct touch_device *dev)
{
        struct cst816t_state *state = (struct cst816t_state *) dev->driver_ctx->state;
        if(!_cst816t_int_asserted(state->pin_int))
                return false;

        uint8_t data[CST816T_TOUCH_DATA_LEN];
        if(!light_display_ioport_read_register(state->io_ctx, CST816T_REG_TOUCH_DATA, data, CST816T_TOUCH_DATA_LEN))
                return false;

        uint8_t finger_num = data[0];
        dev->touch_active = finger_num > 0;
        if(dev->touch_active) {
                dev->x = ((uint16_t)(data[1] & 0x0F) << 8) | data[2];
                dev->y = ((uint16_t)(data[3] & 0x0F) << 8) | data[4];
        }

        if(dev->touch_active && !state->was_active) {
                light_info("touch down: device '%s', x=%d, y=%d", dev->header.id, dev->x, dev->y);
        }
        state->was_active = dev->touch_active;
        return true;
}

struct touch_device *light_touch_cst816t_create_device(
        uint8_t *name, uint16_t x_max, uint16_t y_max, struct io_context *io, uint8_t pin_int)
{
        // io_ctx/pin_int must be attached to the driver state before the device is
        // registered: adding it to the object tree (via light_touch_init_device())
        // immediately triggers init_device(), which reads both -- same rationale/pattern
        // as every display driver's create_device() (see e.g. light_display_sh1106's)
        struct touch_device *dev = light_object_alloc(sizeof(struct touch_device));
        struct touch_driver_context *driver_ctx = _cst816t_spawn_context();
        struct cst816t_state *state = (struct cst816t_state *) driver_ctx->state;
        state->io_ctx = io;
        state->pin_int = pin_int;

        return light_touch_init_device(dev, driver_ctx, x_max, y_max, "%s", name);
}
