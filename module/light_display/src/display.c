#include <light_display.h>

#include "light_display_internal.h"

static void _display_release(struct light_object *obj)
{
        light_free(to_display_device(obj));
}
static struct lobj_type ltype_display_device = (struct lobj_type) {
        .id = ID_DISPLAY_DEVICE,
        .release = _display_release
};

static volatile uint16_t next_device_id;

void light_display_init()
{
        next_device_id = 0;
}
struct display_device *light_display_create_device(struct display_driver *driver, uint8_t *name, uint16_t width,
                                                uint16_t height, uint8_t bpp)
{
        struct display_device *dev = light_object_alloc(sizeof(struct display_device));
        struct display_driver_context *driver_ctx = driver->spawn_context();
        return light_display_init_device(dev, driver_ctx, name, width, height, bpp);
}
struct display_device *light_display_init_device(
                struct display_device *dev,
                struct display_driver_context *driver_ctx,
                uint8_t *name, uint16_t width, uint16_t height, uint8_t bpp)
{
        light_object_init(&dev->header, &ltype_display_device);
        dev->device_id = next_device_id++;
        dev->width = width;
        dev->height = height;
        dev->bpp = bpp;
        dev->driver_ctx = driver_ctx;
}
void light_display_set_render_context(struct display_device *dev, struct rend_context *ctx)
{
        dev->render_ctx = ctx;
}
void light_display_update(struct display_device *dev)
{
        dev->driver_ctx->driver->update(dev);
}
void light_display_clear(struct display_device *dev)
{

}
