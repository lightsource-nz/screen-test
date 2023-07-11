#include <screentest.h>

#include "screentest_internal.h"

void screentest_event(const struct light_module *module, uint8_t event);
static uint8_t screentest_main(struct light_application *app);

Light_Application_Define(screentest, screentest_event, screentest_main,
                                &light_framework);

void main()
{
        light_framework_init();
        light_framework_run();

        __breakpoint();
}

void screentest_event(const struct light_module *module, uint8_t event)
{
        switch(event) {
                case LF_EVENT_LOAD:
                break;
                // TODO implement unregister for event hooks
                case LF_EVENT_UNLOAD:
                break; 
        }
}
static uint8_t screentest_main(struct light_application *app)
{
        // TODO add tick counter and cycle timer, etc
        light_info("enter Screentest application task, time=%dms, time since last run=%dms", 0, 0);
        
        return LF_STATUS_RUN;
}
