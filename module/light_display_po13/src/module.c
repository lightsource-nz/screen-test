#include <light_display_po13.h>

#include "light_display_po13_internal.h"

static void _module_event(const struct light_module *module, uint8_t event);
Light_Module_Define(light_display_po13, _module_event,
                                &light_display,
                                &light_framework)

static void _module_event(const struct light_module *module, uint8_t event)
{
}