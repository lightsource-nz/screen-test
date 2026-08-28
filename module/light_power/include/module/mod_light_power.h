#ifndef _MOD_LIGHT_POWER_H
#define _MOD_LIGHT_POWER_H

#include <light.h>

// the repository's version, derived from its git tags -- see light_project_version(SCREENTEST)
#include <screentest_version.h>
#define LIGHT_POWER_VERSION_STR           SCREENTEST_VERSION_STRING

#define LIGHT_POWER_INFO_STR              "light_power v" LIGHT_POWER_VERSION_STR

Light_Module_Declare(light_power);

#endif
