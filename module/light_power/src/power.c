#include <light_power.h>

#include "light_power_internal.h"

static void _device_root_child_add(struct light_object *obj, struct light_object *child)
{
        struct power_device_root *root = to_power_device_root(obj);
        struct power_device *dev = to_power_device(child);
        root->device[dev->device_id] = dev;
}
static void _device_release(struct light_object *obj)
{
        struct power_device *dev = to_power_device(obj);
        //   the driver context was spawned for this device alone, so it goes with it --
        // without this the device is reclaimed and its context is not, a leak that only
        // shows up once teardown is exercised
        if(dev->driver_ctx && dev->driver_ctx->driver->destroy_context)
                dev->driver_ctx->driver->destroy_context(dev->driver_ctx);
        light_free(dev);
}
static void _device_add(struct light_object *obj, struct light_object *parent)
{
        struct power_device *dev = to_power_device(obj);
        light_debug("name=%s", dev->header.id);
        light_power_command_init(dev);
}
static struct lobj_type ltype_power_device_root = (struct lobj_type) {
        .id = ID_POWER_DEVICE_ROOT,
        .release = NULL,
        .evt_child_add = _device_root_child_add
};
static struct lobj_type ltype_power_device = (struct lobj_type) {
        .id = ID_POWER_DEVICE,
        .release = _device_release,
        .evt_add = _device_add
};
static struct power_device_root device_root;

static volatile uint16_t next_device_id;

void light_power_init()
{
        next_device_id = 0;
        light_object_init_static(&device_root.header, &ltype_power_device_root);
        light_object_add(&device_root.header, NULL, "root_device");
}
struct power_device_root *light_power_device_get_root()
{
        return &device_root;
}
struct power_device *light_power_create_device(struct power_driver *driver, uint8_t *format, ...)
{
        struct power_device *dev = light_object_alloc(sizeof(struct power_device));
        struct power_driver_context *driver_ctx = driver->spawn_context();

        va_list vargs;

        va_start(vargs, format);
        return light_power_init_device_va(dev, driver_ctx, format, vargs);
        va_end(vargs);
}
struct power_device *light_power_init_device(
                struct power_device *dev,
                struct power_driver_context *driver_ctx,
                uint8_t *format, ...)
{
        va_list vargs;

        va_start(vargs, format);
        return light_power_init_device_va(dev, driver_ctx, format, vargs);
        va_end(vargs);
}
struct power_device *light_power_init_device_va(
                struct power_device *dev,
                struct power_driver_context *driver_ctx,
                uint8_t *format, va_list args)
{
        light_trace("(driver=%s, pd=%d)",
                        driver_ctx->driver->name, driver_ctx->driver->pd_compliant);
        // TODO: this should be an ASSERT statement
        if(next_device_id >= LIGHT_POWER_MAX_DEVICES) {
                light_error("could not create new device: max devices reached (%d)", next_device_id);
                return NULL;
        }
        uint8_t device_id = next_device_id++;
        light_object_init(&dev->header, &ltype_power_device);
        dev->device_id = device_id;
        dev->driver_ctx = driver_ctx;

        //   light_object_alloc() does not zero, so every field is set explicitly here, same
        // as every other driver-backed device in this codebase. It matters more than usual
        // for this one: an uninitialised profile[] would advertise garbage VOLTAGES as
        // available, and the first thing a consumer does with an available profile is ask
        // for it
        dev->profile_count = 0;
        for(uint8_t i = 0; i < LIGHT_POWER_MAX_PROFILES; i++) {
                dev->profile[i].millivolts = 0;
                dev->profile[i].milliamps = 0;
                dev->profile[i].available = false;
        }
        dev->contract_active = false;
        dev->active_mv = 0;
        dev->active_ma = 0;
        dev->requested = LIGHT_POWER_PROFILE_NONE;
        //   the ceiling starts where it is safe rather than where the hardware could go: see
        // LIGHT_POWER_SAFE_MAX_MV. A device that has not been told what it is wired to may ask
        // for nothing beyond what is already on the rail
        dev->max_millivolts = LIGHT_POWER_SAFE_MAX_MV;
        dev->last_poll_ms = 0;
        dev->poll_interval_ms = LIGHT_POWER_DEFAULT_POLL_INTERVAL_MS;

        light_object_add_va(&dev->header, &device_root.header, format, args);
        return dev;
}
void light_power_command_init(struct power_device *dev)
{
        light_debug("device: %s", dev->header.id);
        dev->driver_ctx->driver->init_device(dev);
}
void light_power_command_reset(struct power_device *dev)
{
        light_debug("device: %s", dev->header.id);
        dev->driver_ctx->driver->reset(dev);
}
bool light_power_command_poll(struct power_device *dev)
{
        uint32_t now = light_platform_get_time_since_init();

        //   throttled here rather than in each driver, exactly as light_imu throttles its
        // own: the module task runs every scheduler tick and a power contract does not change
        // on that timescale, so the great majority of calls return without touching the bus
        if(dev->poll_interval_ms && now - dev->last_poll_ms < dev->poll_interval_ms)
                return false;
        dev->last_poll_ms = now;

        return dev->driver_ctx->driver->poll(dev);
}
void light_power_set_poll_interval(struct power_device *dev, uint16_t interval_ms)
{
        dev->poll_interval_ms = interval_ms;
}
void light_power_set_max_millivolts(struct power_device *dev, uint16_t millivolts)
{
        //   logged, and at WARNING when the ceiling goes UP: raising it is the moment someone
        // asserted that this rail's downstream can take more, and if that assertion is wrong
        // the evidence is a dead board with nothing in the log to say who claimed otherwise
        if(millivolts > dev->max_millivolts)
                light_warn("device '%s': request ceiling raised %dmV -> %dmV",
                                dev->header.id, dev->max_millivolts, millivolts);
        else
                light_info("device '%s': request ceiling set to %dmV",
                                dev->header.id, millivolts);
        dev->max_millivolts = millivolts;
}
uint16_t light_power_get_max_millivolts(struct power_device *dev)
{
        return dev->max_millivolts;
}

uint8_t light_power_profile_count(struct power_device *dev)
{
        return dev->profile_count;
}
bool light_power_get_profile(struct power_device *dev, uint8_t index, struct power_profile *out)
{
        if(index >= dev->profile_count)
                return false;
        if(out)
                *out = dev->profile[index];
        return true;
}
uint8_t light_power_find_profile(struct power_device *dev, uint16_t max_millivolts)
{
        uint8_t best = LIGHT_POWER_PROFILE_NONE;
        uint16_t best_mv = 0;

        //   the device's ceiling binds regardless of what the caller asked for, so this can
        // never hand back an index that select_profile() would refuse -- see the header
        if(max_millivolts > dev->max_millivolts)
                max_millivolts = dev->max_millivolts;

        for(uint8_t i = 0; i < dev->profile_count; i++) {
                //   unavailable entries are skipped rather than treated as absent, which is
                // the point of keeping them in the list -- see struct power_profile
                if(!dev->profile[i].available)
                        continue;
                if(dev->profile[i].millivolts > max_millivolts)
                        continue;
                if(dev->profile[i].millivolts <= best_mv && best != LIGHT_POWER_PROFILE_NONE)
                        continue;
                best = i;
                best_mv = dev->profile[i].millivolts;
        }
        return best;
}
bool light_power_get_active(struct power_device *dev,
                                uint16_t *millivolts_out, uint16_t *milliamps_out)
{
        //   written unconditionally: the caller asked what the rail is doing, and "nothing was
        // negotiated" is not a reason to withhold the answer -- see the header
        if(millivolts_out) *millivolts_out = dev->active_mv;
        if(milliamps_out)  *milliamps_out = dev->active_ma;
        return dev->contract_active;
}
uint8_t light_power_get_requested(struct power_device *dev)
{
        return dev->requested;
}
bool light_power_is_pd(struct power_device *dev)
{
        return dev->driver_ctx->driver->pd_compliant;
}
bool light_power_select_profile(struct power_device *dev, uint8_t index)
{
        const struct power_driver *drv = dev->driver_ctx->driver;

        if(!drv->select_profile) {
                light_warn("device '%s' reports its supply but cannot be asked to change it",
                                dev->header.id);
                return false;
        }
        if(index >= dev->profile_count) {
                light_error("device '%s': no profile %d (there are %d)",
                                dev->header.id, index, dev->profile_count);
                return false;
        }
        //   refused rather than attempted, because an unavailable profile is a request the
        // source has already said it cannot honour. Sending it anyway would at best be
        // ignored and at worst trigger a renegotiation that drops the rail to nothing on the
        // way to failing
        if(!dev->profile[index].available) {
                light_error("device '%s': profile %d (%dmV) is not currently offered",
                                dev->header.id, index, dev->profile[index].millivolts);
                return false;
        }

        //   the ceiling, checked last because it is the one refusal that is about the BOARD
        // rather than about the source. Everything above asks "can this be had"; this asks
        // "can what is downstream survive it", and the answer is a fact about wiring that this
        // layer was told once and the caller may never have known. Logged as an error rather
        // than passed over quietly: a request this far out of range is a bug in the caller,
        // and the numbers are what make it obvious which one
        if(dev->profile[index].millivolts > dev->max_millivolts) {
                light_error("device '%s': REFUSED profile %d (%dmV) -- this device is limited"
                                " to %dmV. Raise it with light_power_set_max_millivolts() only"
                                " if everything on this rail tolerates the higher voltage",
                                dev->header.id, index, dev->profile[index].millivolts,
                                dev->max_millivolts);
                return false;
        }

        //   logged at INFO unconditionally, and deliberately: this is the one call here that
        // changes the voltage on a live rail, so the log should say it happened even when
        // nothing goes wrong. A rail that moved with no record of anyone asking is a bad
        // thing to debug
        light_info("device '%s': requesting profile %d (%dmV, %dmA)",
                        dev->header.id, index,
                        dev->profile[index].millivolts, dev->profile[index].milliamps);

        //   recorded BEFORE the driver call rather than after: the request has been made
        // whether or not the hardware accepts it, and a consumer comparing `requested`
        // against the active point is asking "did what I asked for happen", which needs the
        // asking recorded even -- especially -- when it did not
        dev->requested = index;
        return drv->select_profile(dev, index);
}
void light_power_poll_devices(void)
{
        for(uint16_t i = 0; i < next_device_id; i++) {
                struct power_device *dev = device_root.device[i];
                if(!dev)
                        continue;
                light_power_command_poll(dev);
        }
}
