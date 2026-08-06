#include <light_touch.h>

#include "light_touch_internal.h"

static void _device_root_child_add(struct light_object *obj, struct light_object *child)
{
        struct touch_device_root *root = to_touch_device_root(obj);
        struct touch_device *dev = to_touch_device(child);
        root->device[dev->device_id] = dev;
}
static void _device_release(struct light_object *obj)
{
        light_free(to_touch_device(obj));
}
static void _device_add(struct light_object *obj, struct light_object *parent) {
        struct touch_device *dev = to_touch_device(obj);
        light_debug("name=%s", dev->header.id);
        light_touch_command_init(dev);
}
// singleton container object for touch_device objects
static struct lobj_type ltype_touch_device_root = (struct lobj_type) {
        .id = ID_TOUCH_DEVICE_ROOT,
        .release = NULL,
        .evt_child_add = _device_root_child_add
};
static struct lobj_type ltype_touch_device = (struct lobj_type) {
        .id = ID_TOUCH_DEVICE,
        .release = _device_release,
        .evt_add = _device_add
};
static struct touch_device_root device_root;

static volatile uint16_t next_device_id;

void light_touch_init()
{
        next_device_id = 0;
        light_object_init(&device_root.header, &ltype_touch_device_root);
        light_object_add(&device_root.header, NULL, "root_device");
}
struct touch_device_root *light_touch_device_get_root()
{
        return &device_root;
}
struct touch_device *light_touch_create_device(struct touch_driver *driver,
                                                uint16_t x_max, uint16_t y_max, uint8_t *format, ...)
{
        struct touch_device *dev = light_object_alloc(sizeof(struct touch_device));
        struct touch_driver_context *driver_ctx = driver->spawn_context();

        va_list vargs;

        va_start(vargs, format);
        return light_touch_init_device_va(dev, driver_ctx, x_max, y_max, format, vargs);
        va_end(vargs);
}
struct touch_device *light_touch_init_device(
                struct touch_device *dev,
                struct touch_driver_context *driver_ctx,
                uint16_t x_max, uint16_t y_max, uint8_t *format, ...)
{
        va_list vargs;

        va_start(vargs, format);
        return light_touch_init_device_va(dev, driver_ctx, x_max, y_max, format, vargs);
        va_end(vargs);
}
struct touch_device *light_touch_init_device_va(
                struct touch_device *dev,
                struct touch_driver_context *driver_ctx,
                uint16_t x_max, uint16_t y_max, uint8_t *format, va_list args)
{
        light_trace("(driver=%s, x_max=%d, y_max=%d)",
                                driver_ctx->driver->name, x_max, y_max);
        // TODO: this should be an ASSERT statement
        if(next_device_id >= LIGHT_TOUCH_MAX_DEVICES) {
                light_error("could not create new device: max devices reached (%d)", next_device_id);
                return NULL;
        }
        uint8_t device_id = next_device_id++;
        light_object_init(&dev->header, &ltype_touch_device);
        dev->device_id = device_id;
        dev->x_max = x_max;
        dev->y_max = y_max;
        dev->touch_active = false;
        dev->x = 0;
        dev->y = 0;
        dev->driver_ctx = driver_ctx;

        light_object_add_va(&dev->header, &device_root.header, format, args);
        return dev;
}
void light_touch_command_init(struct touch_device *dev)
{
        light_debug("device: %s", dev->header.id);
        dev->driver_ctx->driver->init_device(dev);
}
void light_touch_command_reset(struct touch_device *dev)
{
        light_debug("device: %s", dev->header.id);
        dev->driver_ctx->driver->reset(dev);
}
bool light_touch_command_poll(struct touch_device *dev, uint16_t *x_out, uint16_t *y_out)
{
        bool got_sample = dev->driver_ctx->driver->poll(dev);
        if(got_sample && dev->touch_active) {
                if(x_out) *x_out = dev->x;
                if(y_out) *y_out = dev->y;
        }
        return got_sample;
}
void light_touch_poll_devices(void)
{
        for(uint16_t i = 0; i < next_device_id; i++) {
                struct touch_device *dev = device_root.device[i];
                if(!dev)
                        continue;
                dev->driver_ctx->driver->poll(dev);
        }
}
