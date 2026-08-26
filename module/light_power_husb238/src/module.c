#include <light_power_husb238.h>

// for Light_Module_Declare(light_power) -- named in the dependency list below, and a module
// symbol has to be declared before it can be pointed at
#include <module/mod_light_power.h>

//   a driver module with no load-time work of its own: the device is created by the
// application's hardware wiring, which is what knows the io_context. Declared as a module
// all the same, so an app naming it in Light_Application_Define() pulls in light_power with
// it -- the dependency that actually matters, since light_power owns the polling task
static void _module_event(const struct light_module *module, uint8_t event, void *arg);
Light_Module_Define(light_power_husb238, _module_event,
                                &light_power,
                                &light_core);

static void _module_event(const struct light_module *module, uint8_t event, void *arg)
{
        switch(event) {
                case LF_EVENT_MODULE_LOAD:
                break;
                case LF_EVENT_MODULE_UNLOAD:
                break;
        }
}
