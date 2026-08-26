#ifndef _LIGHT_POWER_H
#define _LIGHT_POWER_H

#include <light.h>
#include <light_ioport.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#define ID_POWER_DEVICE_ROOT                    "light_power:device_root"
#define ID_POWER_DEVICE                         "light_power:device"

#define LIGHT_POWER_MAX_DEVICES                 4
//   the most operating points one device may offer. Six covers USB PD's fixed supply
// voltages (5/9/12/15/18/20V) with room for a couple more; a device offering more than this
// reports the first LIGHT_POWER_MAX_PROFILES of them rather than overrunning
#define LIGHT_POWER_MAX_PROFILES                8
// returned by the selection helpers when no profile matches, and held in dev->requested
// until something has actually been asked for
#define LIGHT_POWER_PROFILE_NONE                0xFF

//   the ceiling a device starts with: the USB-C default rail, and the only voltage a source
// supplies before anyone negotiates for more.
//
//   SAFE BY DEFAULT IS THE POINT. Selecting a profile moves a real rail, and what hangs off
// that rail is a fact about the board that this layer cannot see. The failure is not
// theoretical and not recoverable: on the bench this was written against, the sink's output is
// hardwired to a Pico's 5V input, so a single successful request for 9V ends the Pico. A
// library whose default lets one call do that is wrong regardless of how carefully its callers
// behave.
//   so the default permits exactly what is already there, and anything more is an explicit
// decision made through light_power_set_max_millivolts() by whoever knows the wiring
#define LIGHT_POWER_SAFE_MAX_MV                 5000

//   what became of the last request. A selection is not a function call that succeeds or
// fails -- it is a message handed to a negotiation that answers later, or does not answer at
// all -- so the outcome is a STATE the poll path resolves rather than a return value.
//
//   the case that forced this: a HUSB238 was asked for 15V and for 20V, both advertised by the
// source. Both writes landed, both returned success, and the source granted neither -- the
// contract simply stayed where it was. A refused request is indistinguishable from a
// successful one at the moment of asking, because nothing has happened yet either way
#define LIGHT_POWER_REQUEST_NONE                0
#define LIGHT_POWER_REQUEST_PENDING             1
#define LIGHT_POWER_REQUEST_ACTIVE              2
#define LIGHT_POWER_REQUEST_REFUSED             3
//   how long a request may stay PENDING before it is called refused. USB PD negotiation
// completes in well under a second -- 12V was measured landing inside 800ms on this bench --
// and a request still unanswered at this point was measured still unanswered at 2s, so waiting
// longer only delays the news
#define LIGHT_POWER_REQUEST_TIMEOUT_MS          1500

//   one selectable operating point: a voltage the source can be asked to supply, and the
// most current it will provide there.
//
//   `available` is separate from the entry existing at all, and that distinction is the
// whole reason profiles are indexed rather than compacted. A USB PD source advertises a
// FIXED set of voltages and marks which of them it actually offers -- a 45W charger with no
// 18V rail still has an 18V entry, it is simply not on offer. Compacting the list would
// renumber every profile above the missing one whenever a different supply was plugged in,
// which would silently turn a stored "profile 4" into a request for a different voltage.
// So indices are stable and availability is a property of the entry
struct power_profile {
        uint16_t millivolts;
        uint16_t milliamps;
        bool available;
};

struct power_device;
struct power_driver
{
        const uint8_t *name;
        //   whether the part underneath genuinely implements USB Power Delivery, as opposed
        // to merely offering a set of voltages this abstraction can select between.
        //
        //   this is not cosmetic, because it changes what a profile MEANS. On a PD device the
        // profiles are a remote source's advertised capabilities and the active operating
        // point is a negotiated contract: the source can withdraw an offer, refuse a request,
        // or renegotiate without being asked, so a consumer must treat the list as a snapshot
        // and re-read it. On a non-PD device the profiles are local settings that do not
        // change underneath the caller, and a successful selection stays selected.
        //   a consumer that needs to know reads it through light_power_is_pd()
        bool pd_compliant;

        struct power_driver_context *(*spawn_context)();
        //   frees whatever spawn_context() allocated, called when the device holding that
        // context is released. OPTIONAL: a driver whose context is not heap-allocated leaves
        // this NULL and the release path skips it
        void (*destroy_context)(struct power_driver_context *ctx);
        void (*init_device)(struct power_device *);
        void (*reset)(struct power_device *);
        //   refreshes dev->profile[] and the active operating point from the hardware.
        // Returns true if the device answered. A plain synchronous read: a handful of
        // single-byte register reads, nothing like a display frame transfer
        bool (*poll)(struct power_device *);
        //   asks the hardware to switch to profile[index], which the caller has already
        // checked is available. Returns true if the request was ACCEPTED FOR SENDING, which
        // is not the same as the new voltage being in force -- on a PD device the contract is
        // negotiated asynchronously and only a later poll() can confirm it landed.
        //   OPTIONAL: NULL on a device that reports what it is supplying but cannot be asked
        // to change it, in which case light_power_select_profile() fails cleanly
        bool (*select_profile)(struct power_device *, uint8_t index);
};
struct power_driver_context
{
        const struct power_driver *driver;
        const void *state;
};

struct power_device {
        struct light_object header;
        uint8_t device_id;
        struct power_driver_context *driver_ctx;

        //   what the source offers, refreshed by poll(). Indices are stable for the life of
        // the device -- see struct power_profile
        struct power_profile profile[LIGHT_POWER_MAX_PROFILES];
        uint8_t profile_count;

        //   what the rail is supplying right now, agreed or not, and separately whether that
        // is the result of an actual negotiation.
        //
        //   the two really are independent, and conflating them was a bug in the first cut of
        // this: a USB-C sink with nothing negotiated still sits at the bus's 5V default, so a
        // device reporting "5V" is not thereby reporting a contract. A consumer that treats a
        // default rail as an agreement is surprised when it moves; one that ignores the rail
        // because there is no contract cannot tell what it is running on. Both are worth
        // knowing and neither implies the other
        bool contract_active;
        uint16_t active_mv;
        uint16_t active_ma;

        //   the profile last REQUESTED through light_power_select_profile(), which is not
        // necessarily the one in force: a PD source may refuse, or take a while. Kept so a
        // consumer can tell "asked for 12V and got it" from "asked for 12V and is still at
        // 5V", which are indistinguishable from active_mv alone
        uint8_t requested;
        //   and what became of it -- one of LIGHT_POWER_REQUEST_*, resolved by the poll path
        // rather than by the call that made the request. request_started_ms is when the asking
        // happened, which is what lets an unanswered request eventually be called refused
        uint8_t request_state;
        uint32_t request_started_ms;

        //   the highest voltage this device may be ASKED for -- a property of what is wired
        // downstream, not of what the source offers, which is why it lives on the device and
        // not in the profile list. Starts at LIGHT_POWER_SAFE_MAX_MV
        uint16_t max_millivolts;

        //   how often poll() actually reaches the hardware. The module task runs every
        // scheduler tick, and a power contract does not change on that timescale -- reading
        // seven registers per tick would be pure bus traffic against a chip sharing its bus
        uint32_t last_poll_ms;
        uint16_t poll_interval_ms;
};
struct power_device_root {
        struct light_object header;
        struct power_device *device[LIGHT_POWER_MAX_DEVICES];
};

#define to_power_device_root(ptr) container_of(ptr, struct power_device_root, header)
#define to_power_device(ptr) container_of(ptr, struct power_device, header)

extern void light_power_init();

extern struct power_device_root *light_power_device_get_root();
extern struct power_device *light_power_create_device(struct power_driver *driver,
                                                uint8_t *format, ...);
//   lower-level entry point, same rationale as light_touch_init_device(): a driver's own
// state (its io_context, say) must be attached to driver_ctx before the device is added to
// the object tree, because that add synchronously triggers init_device()
extern struct power_device *light_power_init_device(
                struct power_device *dev,
                struct power_driver_context *driver_ctx,
                uint8_t *format, ...);
extern struct power_device *light_power_init_device_va(
                struct power_device *dev,
                struct power_driver_context *driver_ctx,
                uint8_t *format, va_list args);

extern void light_power_command_init(struct power_device *dev);
extern void light_power_command_reset(struct power_device *dev);
//   refreshes the device if its poll interval has elapsed. Returns true when the hardware
// was actually read AND answered; false both when the interval has not elapsed and when the
// read failed, which are deliberately not distinguished -- neither is news to a caller, and
// a device that has never answered simply reports no profiles
extern bool light_power_command_poll(struct power_device *dev);
extern void light_power_set_poll_interval(struct power_device *dev, uint16_t interval_ms);

// --- what the source offers ---

extern uint8_t light_power_profile_count(struct power_device *dev);
// false for an out-of-range index, leaving *out untouched
extern bool light_power_get_profile(struct power_device *dev, uint8_t index,
                                                struct power_profile *out);
//   the highest-voltage AVAILABLE profile at or below `max_millivolts`, or
// LIGHT_POWER_PROFILE_NONE. The common ask -- "give me as much as I can safely take" --
// expressed once here rather than as a loop in every consumer.
//   the device's own ceiling applies too, whichever is lower, so this can never name a
// profile that light_power_select_profile() would then refuse. A "find then select" pair that
// disagrees with itself is a trap, and the caller who falls into it is holding a profile index
// it was just handed
extern uint8_t light_power_find_profile(struct power_device *dev, uint16_t max_millivolts);

// --- what this board can survive ---

//   raises or lowers the ceiling on what may be REQUESTED. Starts at LIGHT_POWER_SAFE_MAX_MV;
// raising it asserts that everything downstream of this rail tolerates the new voltage, which
// is a claim about the board that only the board's own wiring code is in a position to make.
// Belongs beside the device's creation, not at a call site that happens to want more power
extern void light_power_set_max_millivolts(struct power_device *dev, uint16_t millivolts);
extern uint16_t light_power_get_max_millivolts(struct power_device *dev);

// --- what is actually in force ---

//   what the rail is supplying right now. ALWAYS writes the outputs -- zero if the device has
// reported nothing yet -- and returns whether that is a NEGOTIATED contract as opposed to the
// bus's default. So false does not mean "no answer": it means "this is what you are running
// on, and nobody agreed to it". Either pointer may be NULL
extern bool light_power_get_active(struct power_device *dev,
                                                uint16_t *millivolts_out, uint16_t *milliamps_out);
extern uint8_t light_power_get_requested(struct power_device *dev);
//   whether the hardware genuinely implements USB Power Delivery -- see pd_compliant in
// struct power_driver for why a consumer might care
extern bool light_power_is_pd(struct power_device *dev);

// --- changing it ---

//   asks for profile[index]. THIS CHANGES THE VOLTAGE ON A LIVE RAIL, which is what makes it
// unlike everything above: every other call here observes, and this one acts on hardware that
// may be powering the caller. Whatever is downstream of that rail must be able to survive the
// new voltage -- this layer cannot know what is, and does not guess.
//   refuses an out-of-range index, a profile the source does not currently offer, a voltage
// above this device's ceiling, and a driver with no selection support -- so the failure modes
// that ARE knowable here are caught before anything reaches the wire.
//
//   RETURNS ONLY THAT THE REQUEST WAS SENT. It cannot mean more than that: the source answers
// on its own schedule, and at the moment of asking a request that will be granted and one that
// will be refused look identical. Measured, not theorised -- a HUSB238 was asked for 15V and
// 20V, both advertised by the source, and both writes landed and returned true while the
// source granted neither.
//   so the OUTCOME arrives through light_power_request_state(), which the poll path resolves
// to ACTIVE or REFUSED. A caller that needs to know it got what it asked for waits for that;
// one that treats this bool as the answer will believe it is running at 20V while sitting at 12
extern bool light_power_select_profile(struct power_device *dev, uint8_t index);
//   one of LIGHT_POWER_REQUEST_*. Resolved by light_power_command_poll(), so a caller that
// never polls sees PENDING forever -- which is honest, since without asking the hardware there
// genuinely is no news
extern uint8_t light_power_request_state(struct power_device *dev);

#endif
