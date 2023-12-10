#include <light.h>
#include <module/mod_light_display.h>

static void _module_event(const struct light_module *module, uint8_t event);
Light_Module_Define(light_display_sh1106, _module_event,
                                &light_display,
                                &light_framework)

static void _module_event(const struct light_module *module, uint8_t event)
{
}