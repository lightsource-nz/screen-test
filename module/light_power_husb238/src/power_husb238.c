#include <light_power_husb238.h>

struct husb238_state {
        struct io_context *io_ctx;
        //   whether the last poll reached the chip, so the transitions can be logged without
        // a line per poll. An unplugged USB-C port is the ORDINARY resting state of this part
        // -- it draws its own power from the supply it negotiates with -- so silence is not
        // an error and must not be logged like one
        bool answering;
};

//   the source voltages the six SRC_PDO registers describe, in the order the registers appear.
// Fixed by the part, not by the supply: a charger that offers none of them still has six
// registers, all with their detect bit clear
static const uint16_t _pdo_millivolts[HUSB238_PDO_COUNT] = {
        5000, 9000, 12000, 15000, 18000, 20000
};

//   the current code table, shared by the SRC_PDO registers and PD_STATUS0. See the header
// for why this is believed correct despite coming from third-party sources: it produces a
// self-consistent 45W at two different voltages on the charger it was checked against, which
// a wrong table would not
static const uint16_t _current_ma[16] = {
        500, 700, 1000, 1250, 1500, 1750, 2000, 2250,
        2500, 2750, 3000, 3250, 3500, 4000, 4500, 5000
};

//   PD_STATUS0's voltage codes, and the codes SRC_PDO_SEL takes -- the same numbering, which
// is what lets a profile index be turned into a selection code by adding one. Index 0 means
// "nothing negotiated", so the six real voltages are codes 1..6
#define HUSB238_VOLTAGE_CODE_NONE               0
#define HUSB238_CODE_FROM_INDEX(i)              ((uint8_t)((i) + 1))

static struct power_driver_context *_husb238_spawn_context();
static void _husb238_destroy_context(struct power_driver_context *ctx);
static void _husb238_init(struct power_device *dev);
static void _husb238_reset(struct power_device *dev);
static bool _husb238_poll(struct power_device *dev);
static bool _husb238_select_profile(struct power_device *dev, uint8_t index);

static struct power_driver _driver_husb238 = {
        .name = "power.driver:husb238",
        //   genuinely a USB PD sink, so its profiles are a remote source's advertised
        // capabilities and its active point is a negotiated contract -- both can change
        // without this device being asked, which is exactly what this flag warns a consumer
        // about. See pd_compliant in struct power_driver
        .pd_compliant = true,
        .spawn_context = _husb238_spawn_context,
        .destroy_context = _husb238_destroy_context,
        .init_device = _husb238_init,
        .reset = _husb238_reset,
        .poll = _husb238_poll,
        .select_profile = _husb238_select_profile
};

struct power_driver *light_power_driver_husb238()
{
        return &_driver_husb238;
}

static struct power_driver_context *_husb238_spawn_context()
{
        struct power_driver_context *ctx = light_alloc(sizeof(struct power_driver_context));
        ctx->driver = light_power_driver_husb238();
        ctx->state = light_alloc(sizeof(struct husb238_state));
        struct husb238_state *state = (struct husb238_state *) ctx->state;
        // light_alloc() does not zero, same as every other driver state here
        state->io_ctx = NULL;
        state->answering = false;
        return ctx;
}
static void _husb238_destroy_context(struct power_driver_context *ctx)
{
        light_free((void *)ctx->state);
        light_free(ctx);
}

// one byte, always -- see the header
static bool _read_reg(struct husb238_state *state, uint8_t reg, uint8_t *out)
{
        return light_ioport_read_register(state->io_ctx, reg, out, 1);
}
//   the STRICTLY FRAMED write, not light_ioport_write_register(). This part wants its register
// address and its payload as one two-byte transaction, and the general path sends them as two
// transfers separated by a repeated START that re-sends the device address. The chip
// acknowledges that and stores nothing: measured here, SRC_PDO_SEL took five writes of five
// different values and read back 0x00 after every one, which is why the negotiation appeared
// to do nothing while every return code said success
static bool _write_reg(struct husb238_state *state, uint8_t reg, uint8_t value)
{
        return light_ioport_write_register_byte(state->io_ctx, reg, value);
}

//   the profile list's SHAPE is fixed by the part, so it is filled in here once rather than
// discovered: six entries at the six voltages, all initially unavailable. poll() then only
// has to update availability and current, which is what actually varies with what is plugged
// in -- and the indices a consumer holds stay meaningful across a change of supply
static void _husb238_init(struct power_device *dev)
{
        struct husb238_state *state = (struct husb238_state *) dev->driver_ctx->state;

        dev->profile_count = HUSB238_PDO_COUNT;
        for(uint8_t i = 0; i < HUSB238_PDO_COUNT; i++) {
                dev->profile[i].millivolts = _pdo_millivolts[i];
                dev->profile[i].milliamps = 0;
                dev->profile[i].available = false;
        }

        //   a presence check, log-and-continue: this part has no ID register, so the closest
        // thing to a probe is whether it answers at all. And it very reasonably may not --
        // with nothing plugged into its USB-C port it has no power and cannot respond, which
        // is a normal state rather than a fault. poll() will find it when it wakes
        uint8_t status0;
        if(_read_reg(state, HUSB238_REG_PD_STATUS0, &status0)) {
                light_info("husb238 answering for device '%s' (PD_STATUS0=%02x)",
                                dev->header.id, status0);
                state->answering = true;
        } else {
                light_info("husb238 not answering for device '%s' yet -- expected when no"
                                " USB-C source is plugged into it", dev->header.id);
        }
}
//   no reset line on this part, and nothing to put back: the only writable state is the PDO
// selection, and clearing it would itself be a renegotiation -- a reset that changes the
// output voltage is not a reset. So this is deliberately a no-op rather than a guess at what
// resetting ought to mean
static void _husb238_reset(struct power_device *dev)
{
        light_debug("husb238 has no reset line; nothing to do for device '%s'", dev->header.id);
}

static bool _husb238_poll(struct power_device *dev)
{
        struct husb238_state *state = (struct husb238_state *) dev->driver_ctx->state;
        uint8_t status0;

        //   PD_STATUS0 first, and its success decides whether the rest is worth attempting:
        // seven failed transactions against an unpowered chip is seven timed-out transfers on
        // a bus that may be shared, where one answers the same question
        if(!_read_reg(state, HUSB238_REG_PD_STATUS0, &status0)) {
                if(state->answering) {
                        state->answering = false;
                        light_info("device '%s' stopped answering -- USB-C source unplugged?",
                                        dev->header.id);
                        //   everything known about the source goes with it. Leaving the last
                        // seen capabilities in place would let a consumer select a profile
                        // from a charger that is no longer attached
                        dev->contract_active = false;
                        dev->active_mv = 0;
                        dev->active_ma = 0;
                        dev->requested = LIGHT_POWER_PROFILE_NONE;
                        for(uint8_t i = 0; i < dev->profile_count; i++) {
                                dev->profile[i].available = false;
                                dev->profile[i].milliamps = 0;
                        }
                }
                return false;
        }
        if(!state->answering) {
                state->answering = true;
                light_info("device '%s' answering again", dev->header.id);
        }

        uint8_t v_code = (uint8_t)(status0 >> HUSB238_STATUS0_VOLTAGE_SHIFT);
        uint8_t i_code = (uint8_t)(status0 & HUSB238_STATUS0_CURRENT_MASK);

        //   what is on the rail, whether or not anyone agreed to it
        if(v_code == HUSB238_VOLTAGE_CODE_NONE || v_code > HUSB238_PDO_COUNT) {
                dev->active_mv = 0;
                dev->active_ma = 0;
        } else {
                dev->active_mv = _pdo_millivolts[v_code - 1];
                dev->active_ma = _current_ma[i_code];
        }

        //   ...and whether it is a NEGOTIATED contract, which PD_STATUS0 alone cannot say.
        // It reports the voltage present, and an unattached sink sits at the USB-C 5V default
        // -- so reading a contract out of "PD_STATUS0 says 5V" reports an agreement that was
        // never made, which is exactly what this abstraction promises not to do.
        //   SRC_PDO_SEL is what distinguishes them: zero until a PDO has actually been
        // requested, and holding the requested code afterwards. Observed 0x00 on a bench
        // where the charger was attached and supplying 5V, with PD_STATUS0 reading 0x13 --
        // the two disagreeing is precisely the case this exists to get right
        uint8_t sel;
        if(!_read_reg(state, HUSB238_REG_SRC_PDO_SEL, &sel))
                return false;
        dev->contract_active = (uint8_t)(sel >> HUSB238_PDO_SEL_SHIFT) != HUSB238_VOLTAGE_CODE_NONE;

        //   the capability list. Read every time rather than once at init, because it belongs
        // to whatever is plugged in at this moment -- swap the charger and these change with
        // no notification of any kind
        for(uint8_t i = 0; i < HUSB238_PDO_COUNT; i++) {
                uint8_t pdo;
                if(!_read_reg(state, (uint8_t)(HUSB238_PDO_FIRST_REG + i), &pdo)) {
                        // a partial read leaves the rest of the list as it was, which is
                        // better than half-clearing it: the next poll will get the truth
                        return false;
                }
                dev->profile[i].available = (pdo & HUSB238_PDO_DETECTED) != 0;
                //   zeroed rather than decoded when the source does not offer this voltage.
                // An absent PDO reads 0x00, whose current code decodes to the table's first
                // entry -- so a straight decode reports "18000mV 500mA, unavailable", and
                // 500mA there is not a small number, it is a meaningless one that looks like
                // a measurement. Nothing should read a current off an entry that is not on
                // offer, and this makes that unmistakable rather than merely documented
                dev->profile[i].milliamps = dev->profile[i].available
                                ? _current_ma[pdo & HUSB238_PDO_CURRENT_MASK] : 0;
        }
        return true;
}

//   the one call in this driver that acts rather than observes. light_power_select_profile()
// has already checked the index is in range and the profile is on offer, so what is left here
// is the two-write sequence the part wants: name the PDO, then tell it to go.
//   NOT verified here, deliberately. The request is handed to a PD negotiation that completes
// in its own time, so reading PD_STATUS0 back immediately would report the OLD contract and
// look like a failure. The next poll() reports what actually happened, which is why
// light_power_select_profile() documents its return as "sent" rather than "in force"
static bool _husb238_select_profile(struct power_device *dev, uint8_t index)
{
        struct husb238_state *state = (struct husb238_state *) dev->driver_ctx->state;
        uint8_t sel = (uint8_t)(HUSB238_CODE_FROM_INDEX(index) << HUSB238_PDO_SEL_SHIFT);

        if(!_write_reg(state, HUSB238_REG_SRC_PDO_SEL, sel)) {
                light_error("device '%s': could not write SRC_PDO_SEL", dev->header.id);
                return false;
        }
        //   order matters and is not interchangeable: GO_COMMAND acts on whatever SRC_PDO_SEL
        // holds at the moment it is written, so a REQUEST_PDO issued first would re-request
        // the PREVIOUS selection -- which succeeds, and leaves the rail at the wrong voltage
        // with every return code saying it worked
        if(!_write_reg(state, HUSB238_REG_GO_COMMAND, HUSB238_GO_REQUEST_PDO)) {
                light_error("device '%s': could not write GO_COMMAND", dev->header.id);
                return false;
        }
        return true;
}

struct power_device *light_power_husb238_create_device(uint8_t *name, struct io_context *io)
{
        //   io_ctx attached to the driver state BEFORE the device is registered: adding it to
        // the object tree triggers init_device() synchronously, which reads it. Same
        // rationale and pattern as every other driver's create_device() here
        struct power_device *dev = light_object_alloc(sizeof(struct power_device));
        struct power_driver_context *driver_ctx = _husb238_spawn_context();
        struct husb238_state *state = (struct husb238_state *) driver_ctx->state;
        state->io_ctx = io;

        return light_power_init_device(dev, driver_ctx, "%s", name);
}
