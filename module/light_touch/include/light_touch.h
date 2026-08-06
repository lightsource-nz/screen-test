#ifndef _LIGHT_TOUCH_H
#define _LIGHT_TOUCH_H

#include <light.h>
#include <light_display_ioport.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#define ID_TOUCH_DEVICE_ROOT                    "light_touch:device_root"
#define ID_TOUCH_DEVICE                         "light_touch:device"

#define LIGHT_TOUCH_MAX_DEVICES                 8

struct touch_device;
struct touch_driver
{
        const uint8_t *name;
        struct touch_driver_context *(*spawn_context)();
        void (*init_device)(struct touch_device *);
        void (*reset)(struct touch_device *);
        // samples the controller for a new touch state, writing into dev->touch_active/
        // x/y. returns true if a new sample was captured, false if there was nothing new
        // to report (e.g. the controller's interrupt line wasn't asserted). a plain
        // synchronous call, not an async/DMA trio like light_display's update -- an I2C
        // touch-data read is a handful of bytes, nothing like a display frame transfer
        bool (*poll)(struct touch_device *);
};
struct touch_driver_context
{
        const struct touch_driver *driver;
        const void *state;
};

struct touch_device {
        struct light_object header;
        uint8_t device_id;
        // coordinate range the controller reports within -- analogous to display's
        // width/height, but there's no bpp/render_ctx equivalent: touch devices don't
        // own a pixel buffer
        uint16_t x_max;
        uint16_t y_max;
        // last-sampled state, updated by poll()
        bool touch_active;
        uint16_t x;
        uint16_t y;
        struct touch_driver_context *driver_ctx;
};
struct touch_device_root {
        struct light_object header;
        struct touch_device *device[LIGHT_TOUCH_MAX_DEVICES];
};

#define to_touch_device_root(ptr) container_of(ptr, struct touch_device_root, header)
#define to_touch_device(ptr) container_of(ptr, struct touch_device, header)

extern void light_touch_init();

extern struct touch_device_root *light_touch_device_get_root();
extern struct touch_device *light_touch_create_device(struct touch_driver *driver,
                                                uint16_t x_max, uint16_t y_max, uint8_t *format, ...);
extern struct touch_device *light_touch_create_device_va(struct touch_driver *driver,
                                                uint16_t x_max, uint16_t y_max, uint8_t *format, va_list args);
// lower-level entry point, same rationale as light_display_init_device(): a driver's own
// state (e.g. its io_context) must be attached to driver_ctx before the device is added
// to the object tree, since that add synchronously triggers init_device() -- too late to
// set driver-private state first if using the one-shot create_device() above
extern struct touch_device *light_touch_init_device(
                struct touch_device *dev,
                struct touch_driver_context *driver_ctx,
                uint16_t x_max, uint16_t y_max,
                uint8_t *format, ...);
extern struct touch_device *light_touch_init_device_va(
                struct touch_device *dev,
                struct touch_driver_context *driver_ctx,
                uint16_t x_max, uint16_t y_max,
                uint8_t *format, va_list args);
extern void light_touch_command_init(struct touch_device *dev);
extern void light_touch_command_reset(struct touch_device *dev);
// polls the device for a new sample. returns true and fills x_out/y_out (if non-NULL)
// when a new touch was captured; returns false otherwise (nothing new since the last
// poll). x_out/y_out are left untouched on a false return
extern bool light_touch_command_poll(struct touch_device *dev, uint16_t *x_out, uint16_t *y_out);

#endif
