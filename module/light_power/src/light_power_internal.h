#ifndef _LIGHT_POWER_INTERNAL_H
#define _LIGHT_POWER_INTERNAL_H

//   how often a device is actually read, unless a consumer overrides it with
// light_power_set_poll_interval(). Half a second: a power contract changes when someone
// plugs or unplugs something, so this only has to be fast enough that a person does not
// notice the lag -- and every tick faster is bus traffic against a chip that has nothing
// new to say
#define LIGHT_POWER_DEFAULT_POLL_INTERVAL_MS    500

//   called once per scheduler tick by light_power's own periodic task (module.c) to refresh
// every live device. Each call is throttled per-device by light_power_command_poll(), so
// this being called at tick rate costs almost nothing. Not meant for application code,
// hence kept out of the public header
extern void light_power_poll_devices(void);

#endif
