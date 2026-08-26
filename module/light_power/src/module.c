#include <light_power.h>

#include "light_power_internal.h"

static void _module_event(const struct light_module *module, uint8_t event, void *arg);
Light_Module_Define(light_power, _module_event,
                                &light_core);

static uint8_t _module_task(struct light_application *app);
static void _module_event(const struct light_module *module, uint8_t event, void *arg)
{
        switch(event) {
                case LF_EVENT_MODULE_LOAD:
                light_power_init();
                light_module_register_periodic_task(&light_power, "light_power_task", _module_task);
                break;
                // TODO implement unregister for event hooks
                case LF_EVENT_MODULE_UNLOAD:
                break;
        }
}
static uint8_t _module_task(struct light_application *app)
{
        light_power_poll_devices();
        return LF_STATUS_RUN;
}
