//   THE CONSOLE ROOT IS `screentest_husb238` NOW, not `husb238`. Light_Application_Define()
// names the root command after the application, with nothing to override, so the two cannot
// drift apart -- at the cost of renaming this rig's console root, which was shorter.
//   in practice nothing typed at the console changes: lines are entered without the root name
// ("scan", "dump", "read 0x09"), and _poll_console() below queues them against the root command
// rather than naming it. Only a line that spelled the root out in full is affected.
#define LIGHT_APP_ROOT_COMMAND_DESCRIPTION      "interrogates the I2C device on port 0"

#include <light.h>
#if(LIGHT_SYSTEM == SYSTEM_PICO_SDK)
// for gpio_get() -- see _check_bus_idle(), which reads the I2C pads directly
#include <hardware/gpio.h>
#endif
#include <light_cli.h>
#include <light_ioport.h>
#include <light_platform.h>
#include <light_power_husb238.h>
#include <module/mod_light_cli.h>
#include <module/mod_light_power.h>
#include <module/mod_light_power_husb238.h>

#include <stdint.h>
#include <string.h>

// app: screentest_husb238
//   an I2C interrogation rig for an unidentified chip on a standard Pico 2, written to answer
// "what is this and what does it say" before any driver exists for it. Deliberately NOT a
// driver: nothing here knows what a register MEANS beyond one decode that is clearly marked as
// provisional, because the point is to establish the facts a driver would be built on.
//
//   the part in front of it reads 'HUSB328' on its lid, which is almost certainly a Hynetek
// HUSB238 USB Type-C Power Delivery sink controller with the 2 and 3 transposed -- its I2C
// address is 0x08, matching what this board was said to answer to, and its register file runs
// 0x00..0x09. That identification is INFERRED, not confirmed, which is exactly what `scan` and
// `dump` are for.
//
//   wiring (Pico 2): SDA on GP4 (pin 6), SCL on GP5 (pin 7), grounds common (pin 8), and 1k
// pull-ups to the Pico's 3V3. NO power connection: the HUSB238 runs off the USB-C supply it is
// negotiating with, so it is DEAD -- and will not acknowledge its address -- unless a USB-C
// source is plugged into it. A scan that finds nothing is that, far more often than it is a
// wiring fault.

#define HUSB238_I2C_ADDR                0x08
// the register file to sweep. 0x09 is the last named register; the sweep runs one past it
// because an unverified map is exactly the kind of thing that is off by one
#define HUSB238_REG_FIRST               0x00
#define HUSB238_REG_LAST                0x0A

//   I2C0 with the SDK's default pins for this board, which is also what every Pico write-up
// for this part uses. No reset line: the chip has none, and LIGHT_IOPORT_PIN_NONE is how
// light_ioport is told not to drive a pin that isn't wired to anything
#define HUSB238_PORT_ID                 PORT_I2C_0
#define HUSB238_PIN_SCL                 5
#define HUSB238_PIN_SDA                 4

//   the addresses a scan sweeps. 0x00-0x07 and 0x78-0x7F are reserved by the I2C
// specification, and the pico-sdk refuses them outright (i2c_reserved_addr()), so sweeping
// them would report failures that mean nothing. 0x08 is the first legal address -- which is
// worth noticing, because it means this chip sits right on the boundary
#define SCAN_ADDR_FIRST                 0x08
#define SCAN_ADDR_LAST                  0x77

//   how long after boot the automatic sweep runs. Long enough for the log transport to be
// carrying -- a sweep whose output is printed before anything is listening has not happened as
// far as anyone can tell
#define HUSB238_AUTOSWEEP_DELAY_MS      1000

static struct io_context *_io;
static bool _swept;
static uint32_t _boot_ms;

//   the sweep's findings, kept in memory as well as logged, because on this rig the log may
// have nowhere to go: a Pico wired to a debug probe over SWD alone has no USB console, and its
// UART only reaches the host if the probe's 3-pin UART header is also wired to GP0/GP1. SWD is
// the one transport that is definitely present -- it is how the image got here -- so the
// results are left where a debugger can read them:
//
//     scripts/debug.ps1 -Target screentest_husb238 -Batch -NoBuild -Attach \
//         -Ex 'print _scan_count','print/x _scan_found','print/x _reg_ok','print/x _reg_val'
//
//   costs a few dozen bytes and removes the rig's dependence on a wire nobody has checked
#define SCAN_MAX_FOUND                  8
static uint8_t _scan_found[SCAN_MAX_FOUND];
static uint8_t _scan_count;
// one entry per register in the swept range; _reg_ok says whether the byte beside it is real
static uint8_t _reg_val[HUSB238_REG_LAST + 1];
static uint8_t _reg_ok[HUSB238_REG_LAST + 1];

//   the same facts as seen through light_power, kept beside the raw ones ON PURPOSE. The raw
// sweep reads registers; the driver interprets them, and a driver whose interpretation has
// quietly drifted from the bytes is exactly the bug this rig exists to catch. Having both in
// memory at once means one gdb read compares them.
//   readable the same way as everything else here:
//     -Ex 'print _pd_profiles','print _pd_active_mv','print _pd_is_pd'
static struct power_device *_pd;
static struct power_profile _pd_profiles[LIGHT_POWER_MAX_PROFILES];
static uint8_t _pd_profile_count;
static uint8_t _pd_contract;            // 1 when a contract is in force, 0 when not
static uint16_t _pd_active_mv;
static uint16_t _pd_active_ma;
static uint8_t _pd_is_pd;               // the driver's pd_compliant flag, as light_power reports it
static uint8_t _pd_best_for_12v;        // light_power_find_profile(dev, 12000), a spot-check of the helper
static uint16_t _pd_max_mv;             // the request ceiling actually in force on this device

// the idle level of each bus line, sampled before anything transacts -- see _check_bus_idle()
static uint8_t _bus_scl_high;
static uint8_t _bus_sda_high;

//   results of the selection-mechanism half-test below. Kept separate from the sweep's
// globals because this one does not run unless someone asks for it
static uint8_t _sel_test_ran;
static uint8_t _sel_before;             // SRC_PDO_SEL as found
static uint8_t _sel_after;              // ...after writing the 5V code
static uint8_t _sel_restored;           // ...after putting it back
static uint8_t _sel_status0_before;     // PD_STATUS0 either side of the write: these MUST match,
static uint8_t _sel_status0_after;      // because nothing should have been negotiated

//   THE NEGOTIATION TEST, and the one switch that decides whether this rig moves a real rail.
//
//   Set only while the sink's output is DISCONNECTED from anything that could be harmed. On
// this bench that is now true -- the output was removed from the Pico's 5V input and the Pico
// runs off the debug probe -- which is what makes a full sweep of the source's voltages safe
// and therefore worth doing. Clear it again the moment that output is wired to anything.
//   compile-time rather than a runtime flag on purpose: a build that must not negotiate should
// not CONTAIN the code that does, so nothing left set in RAM, and no debugger poking at a
// variable, can start one
#define HUSB238_ALLOW_NEGOTIATION_TEST          1

//   how long to let a contract settle before believing what the chip says about it. PD
// negotiation is asynchronous -- the request is handed to a conversation with the source that
// completes in its own time -- so reading PD_STATUS0 immediately after asking reports the OLD
// contract and looks exactly like a refusal
//   2s rather than 800ms. At 800ms the 5/9/12V requests all landed and 15V and 20V did not,
// which is exactly the shape a too-short wait produces at the top of the range -- a bigger
// voltage step is a longer transition. Generous enough that a failure at this length is a
// refusal rather than a stopwatch artefact
#define NEG_SETTLE_MS                           2000

// one row per profile: what was asked for, and what actually turned up
static uint8_t _neg_sent[LIGHT_POWER_MAX_PROFILES];
static uint16_t _neg_result_mv[LIGHT_POWER_MAX_PROFILES];
static uint16_t _neg_result_ma[LIGHT_POWER_MAX_PROFILES];
static uint8_t _neg_result_contract[LIGHT_POWER_MAX_PROFILES];
//   the chip's own registers after each attempt, which is what separates the two ways this
// can fail. SRC_PDO_SEL holding our value while PD_STATUS0 stays put means the selection
// landed and GO_COMMAND did not trigger anything -- the request encoding is wrong. SRC_PDO_SEL
// reading back zero means the write never stuck at all, whatever the ACK said
static uint8_t _neg_sel_after[LIGHT_POWER_MAX_PROFILES];
static uint8_t _neg_status0_after[LIGHT_POWER_MAX_PROFILES];
static uint8_t _neg_status1_after[LIGHT_POWER_MAX_PROFILES];
//   what light_power decided the request came to, per stage. The point of recording it beside
// the raw registers is that it is a CLAIM about them: 5/9/12V should read ACTIVE(2) and 15/20V
// REFUSED(3), and any disagreement between this column and PD_STATUS0 is the abstraction
// getting it wrong rather than the hardware
static uint8_t _neg_state[LIGHT_POWER_MAX_PROFILES];
static uint8_t _neg_stage;
static uint8_t _neg_phase;
static uint32_t _neg_started_ms;
static uint8_t _neg_done;

static void _husb238_event(const struct light_module *module, uint8_t event, void *arg);
static uint8_t _husb238_main(struct light_application *app);

Light_Application_Define(screentest_husb238, _husb238_event, _husb238_main,
                                &light_cli,
                                &light_power,
                                &light_power_husb238,
                                &light_core);

void main(int argc, char **argv)
{
        light_framework_init();
        //   (0, NULL) rather than (argc, argv): this is a bare-metal entry point and nothing
        // sets those, so they hold whatever was left in the argument registers -- and light_cli
        // parses them, where garbage argc is indistinguishable from a real command line
        light_framework_run(0, NULL);
}

//   EVERY read here is one byte, and that is a property of the chip rather than a convenience:
// the HUSB238 is documented to transfer only a single byte per bus transaction, so the
// auto-incrementing multi-byte bursts that light_touch_cst816t and light_imu_qmi8658 both rely
// on would be wrong on this part. light_ioport's write-then-read already emits exactly the
// repeated START this needs; it just has to be asked for one byte at a time.
static bool _read_reg(uint8_t reg, uint8_t *out)
{
        return light_ioport_read_register(_io, reg, out, 1);
}

//   a presence probe for one address: read a single byte and see whether anything acknowledged.
// Reading register 0 rather than issuing a bare address poke, because light_ioport has no
// address-only primitive on the 8-bit path that ends in a STOP -- and a register read is a
// harmless thing to do to an unknown chip in a way that a write never is
static bool _probe_addr(uint8_t addr)
{
        //   the address lives in the io_context, so scanning means retargeting the one context
        // rather than building 112 of them. Restored by the caller: leaving it pointed at
        // whatever the sweep ended on would silently redirect every later command
        uint8_t reg = 0, value;
        _io->io.i2c.addr = addr;
        bool present = light_ioport_read_register(_io, reg, &value, 1);
        return present;
}

//   sweeps the legal address range and reports whatever answers. ALWAYS the first thing to run:
// it separates "the wiring is good and the chip is alive" from every other question, and it
// does so without assuming anything about what the chip is
//   the state of the two bus lines with nothing driving them, read straight off the pads.
// GPIO_IN reflects the pad regardless of which peripheral owns the pin, so this works without
// disturbing the I2C function -- and it turns "the chip is not answering" from a guess about
// wiring into a measurement that says WHICH guess.
//
//   both HIGH is a healthy idle bus: the pull-ups are doing their job and the wires reach the
// Pico, so a silent chip is unpowered, absent, or at another address.
//   either LOW with nothing transacting means the level never gets released -- a missing
// pull-up, a line shorted to ground, or a device clamping it.
//   the asymmetry is the useful part: SDA low alone is the classic stuck-slave, SCL low alone
// points at the wire or the pull-up, and both low says the pull-ups are not there at all
static void _check_bus_idle(void)
{
#if(LIGHT_SYSTEM == SYSTEM_PICO_SDK)
        _bus_scl_high = gpio_get(HUSB238_PIN_SCL) ? 1 : 0;
        _bus_sda_high = gpio_get(HUSB238_PIN_SDA) ? 1 : 0;
#endif
        light_info("bus idle levels: SCL(gp%d)=%s SDA(gp%d)=%s",
                        HUSB238_PIN_SCL, _bus_scl_high ? "HIGH" : "LOW",
                        HUSB238_PIN_SDA, _bus_sda_high ? "HIGH" : "LOW");
        if(_bus_scl_high && _bus_sda_high) {
                light_info("  bus is idle and pulled up -- wiring and pull-ups are good, so a"
                                " silent chip is unpowered or absent");
                return;
        }
        if(!_bus_scl_high && !_bus_sda_high)
                light_warn("  BOTH low: pull-ups are not reaching these pins, or both lines are"
                                " shorted to ground");
        else if(!_bus_sda_high)
                light_warn("  SDA held low: a device is clamping it, or SDA is shorted");
        else
                light_warn("  SCL held low: check that wire and its pull-up");
}

static bool _sweep_addresses(void)
{
        uint8_t found = 0;

        light_info("scanning 0x%02x..0x%02x on i2c port %d (scl=%d, sda=%d)",
                        SCAN_ADDR_FIRST, SCAN_ADDR_LAST, HUSB238_PORT_ID,
                        HUSB238_PIN_SCL, HUSB238_PIN_SDA);
        _scan_count = 0;
        for(uint8_t addr = SCAN_ADDR_FIRST; addr <= SCAN_ADDR_LAST; addr++) {
                if(!_probe_addr(addr))
                        continue;
                found++;
                if(_scan_count < SCAN_MAX_FOUND)
                        _scan_found[_scan_count++] = addr;
                light_info("  0x%02x: acknowledged%s", addr,
                                addr == HUSB238_I2C_ADDR ? "  <-- expected HUSB238" : "");
        }
        _io->io.i2c.addr = HUSB238_I2C_ADDR;

        if(!found) {
                light_warn("nothing answered. In order of likelihood: no USB-C source plugged"
                        " into the board (the chip is powered from it, not from the Pico),"
                        " SDA/SCL swapped, no common ground, or pull-ups missing");
                return false;
        }
        light_info("%d device(s) answered", found);
        return true;
}

//   the whole register file, one byte per transaction, raw. Raw FIRST and decoded second is
// deliberate: the map this sweep is checking came from third-party libraries rather than a
// datasheet we hold, so the bytes are the evidence and any interpretation of them is a claim
static bool _dump_registers(void)
{
        static const char *const _name[] = {
                "PD_STATUS0", "PD_STATUS1", "SRC_PDO_5V", "SRC_PDO_9V", "SRC_PDO_12V",
                "SRC_PDO_15V", "SRC_PDO_18V", "SRC_PDO_20V", "SRC_PDO_SEL", "GO_COMMAND"
        };
        uint8_t answered = 0;

        for(uint8_t reg = HUSB238_REG_FIRST; reg <= HUSB238_REG_LAST; reg++) {
                uint8_t value;
                const char *name = reg < (uint8_t)(sizeof(_name) / sizeof(_name[0]))
                                ? _name[reg] : "(past the known map)";
                if(!_read_reg(reg, &value)) {
                        _reg_ok[reg] = 0;
                        _reg_val[reg] = 0;
                        light_warn("  0x%02x %-20s --  no answer", reg, name);
                        continue;
                }
                _reg_ok[reg] = 1;
                _reg_val[reg] = value;
                answered++;
                light_info("  0x%02x %-20s %02x", reg, name, value);
        }
        if(!answered) {
                light_warn("no register answered -- the chip is not responding at 0x%02x",
                                HUSB238_I2C_ADDR);
                return false;
        }
        return true;
}

//   one register, for poking at something the dump made interesting. Takes hex with or without
// an 0x prefix, because a register address is a hex quantity everywhere it is written down and
// asking for decimal here invites a transcription error
static uint8_t _parse_hex(const uint8_t *s, bool *ok)
{
        uint32_t value = 0;
        *ok = false;
        if(!s || !*s)
                return 0;
        if(s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
                s += 2;
        if(!*s)
                return 0;
        for(const uint8_t *p = s; *p; p++) {
                uint8_t digit;
                if(*p >= '0' && *p <= '9')       digit = (uint8_t)(*p - '0');
                else if(*p >= 'a' && *p <= 'f')  digit = (uint8_t)(*p - 'a' + 10);
                else if(*p >= 'A' && *p <= 'F')  digit = (uint8_t)(*p - 'A' + 10);
                else return 0;
                value = (value << 4) | digit;
                if(value > 0xFF)
                        return 0;
        }
        *ok = true;
        return (uint8_t)value;
}
static struct light_cli_invocation_result do_cmd_husb238_read(struct light_cli_invocation *invoke)
{
        const uint8_t *arg = light_cli_invocation_get_arg_value(invoke, 0);
        bool ok;
        uint8_t reg = _parse_hex(arg, &ok);

        if(!ok) {
                light_error("read: expected a register address in hex, e.g. 'read 00'");
                return Result_Error;
        }

        uint8_t value;
        if(!_read_reg(reg, &value)) {
                light_warn("read: register 0x%02x did not answer", reg);
                return Result_Error;
        }
        light_info("read: 0x%02x = %02x", reg, value);
        return Result_Success;
}

//   the ONE decode in this rig, and it is provisional. PD_STATUS0's high nibble is reported by
// several independent third-party libraries as the negotiated voltage and its low nibble as the
// current, but we hold no datasheet confirming it -- so this prints its own reasoning rather
// than an answer, and the raw byte alongside. If the voltage it names disagrees with what a
// meter says is on the supply rail, believe the meter and the map is wrong.
//   the current nibble is deliberately NOT decoded: its code table is the part the sources
// agree on least, and a fabricated amperage is worse than a hex digit
static bool _read_status(void)
{
        static const uint16_t _volts[] = { 0, 5, 9, 12, 15, 18, 20 };
        uint8_t status0;

        if(!_read_reg(0x00, &status0)) {
                light_warn("status: PD_STATUS0 did not answer");
                return false;
        }

        uint8_t v_code = (uint8_t)(status0 >> 4);
        uint8_t i_code = (uint8_t)(status0 & 0x0F);

        light_info("PD_STATUS0 = %02x  (voltage code %x, current code %x)",
                        status0, v_code, i_code);
        if(v_code && v_code < (uint8_t)(sizeof(_volts) / sizeof(_volts[0])))
                light_info("  provisional decode: %dV negotiated", _volts[v_code]);
        else if(!v_code)
                light_info("  provisional decode: no contract negotiated");
        else
                light_info("  voltage code %x is outside the map we have -- the map is"
                                " probably wrong, or this is not a HUSB238", v_code);
        return true;
}

//   thin CLI wrappers over the three above. They exist for a board with a console attached;
// the automatic sweep in _husb238_main() calls the same helpers, so an interactive session and
// a bare SWD-docked board see identical output
static struct light_cli_invocation_result do_cmd_husb238_scan(struct light_cli_invocation *invoke)
{
        return _sweep_addresses() ? Result_Success : Result_Error;
}
static struct light_cli_invocation_result do_cmd_husb238_dump(struct light_cli_invocation *invoke)
{
        return _dump_registers() ? Result_Success : Result_Error;
}
static struct light_cli_invocation_result do_cmd_husb238_status(struct light_cli_invocation *invoke)
{
        return _read_status() ? Result_Success : Result_Error;
}

//   the root command is declared by Light_Application_Define(), named by
// LIGHT_APP_ROOT_COMMAND_NAME at the top of this file, so these hang off LIGHT_COMMAND_APP_ROOT.
// do_cmd_husb238() is gone with it: it printed the subcommand list, which is exactly what the
// default root handler does.
Light_Command_Define(cmd_husb238_scan, LIGHT_COMMAND_APP_ROOT, "scan",
                        "sweeps the bus and reports which addresses answer", do_cmd_husb238_scan, 0, 0);
Light_Command_Define(cmd_husb238_dump, LIGHT_COMMAND_APP_ROOT, "dump",
                        "reads every register in the known map, one byte each", do_cmd_husb238_dump, 0, 0);
Light_Command_Define(cmd_husb238_read, LIGHT_COMMAND_APP_ROOT, "read",
                        "reads one register: read <hex address>", do_cmd_husb238_read, 1, 1);
Light_Command_Define(cmd_husb238_status, LIGHT_COMMAND_APP_ROOT, "status",
                        "reads PD_STATUS0 and decodes it provisionally", do_cmd_husb238_status, 0, 0);

//   NO write command, and that is a deliberate omission rather than an unfinished one. Every
// register worth writing on this part changes a power contract: GO_COMMAND (0x09) triggers a
// renegotiation or a hard reset, and SRC_PDO_SEL (0x08) selects a different output voltage.
// Against a chip whose identity is still inferred, a write is how you drop the supply out from
// under whatever is plugged into it -- or ask for 20V from something that is not expecting it.
// Reads cannot do that. Once the identification is confirmed this is the first thing to add.

#if LIGHT_PLATFORM_USB_ON_CORE1
//   the console feeder: pops at most one completed line per tick from the core 1 USB worker,
// which does the reading and line editing at its end, and hands it to light_cli to dispatch.
// Lines are typed without the root's name -- "scan", "dump", "read 00"
static void _poll_console(void)
{
        uint8_t line[LIGHT_STREAM_MAX_MSG_LENGTH];
        if(!light_core_port_console_take_line(line, sizeof(line)))
                return;
        light_cli_queue_line(light_command_app_root(), line);
}
#else
static void _poll_console(void) {}
#endif

static void _husb238_event(const struct light_module *module, uint8_t event, void *arg)
{
        switch(event) {
        case LF_EVENT_MODULE_LOAD:
                _io = light_ioport_setup_io_i2c(
                                HUSB238_PORT_ID, LIGHT_IOPORT_PIN_NONE, HUSB238_I2C_ADDR,
                                HUSB238_PIN_SCL, HUSB238_PIN_SDA);
                //   its OWN io_context, not _io: the raw sweep retargets _io's address field
                // while scanning, and a driver sharing that context would find itself pointed
                // at whatever the sweep was probing. Two contexts on one peripheral is the
                // normal arrangement anyway -- an io_context carries an address, not a bus
                _pd = light_power_husb238_create_device("husb238_main",
                                light_ioport_setup_io_i2c(
                                        HUSB238_PORT_ID, LIGHT_IOPORT_PIN_NONE,
                                        HUSB238_I2C_ADDR, HUSB238_PIN_SCL, HUSB238_PIN_SDA));
                //   THE WIRING FACT THAT MATTERS ON THIS BENCH: the sink's output is
                // hardwired to the Pico's 5V input. There is no regulator between them and no
                // margin -- a successful request for 9V puts 9V on a 5V rail and the Pico is
                // gone, along with the debug probe's view of it.
                //   set explicitly rather than left to the default, even though the default is
                // already this value. The number is not a preference, it is a description of
                // how these two boards are joined, and it belongs where a reader learns that
                // -- next to the device being created. Should this rail ever feed something
                // that can take more, THIS is the line to change, and changing it is a claim
                // about solder rather than about software
                if(_pd)
                        light_power_set_max_millivolts(_pd, 5000);
                light_info("husb238 probe ready on i2c port %d (scl=%d, sda=%d), address 0x%02x",
                                HUSB238_PORT_ID, HUSB238_PIN_SCL, HUSB238_PIN_SDA,
                                HUSB238_I2C_ADDR);
                light_info("the chip needs a USB-C source plugged into it to answer at all;"
                                " sweeping in %dms", HUSB238_AUTOSWEEP_DELAY_MS);
                _swept = false;
                _boot_ms = light_platform_get_time_since_init();
                break;
        case LF_EVENT_MODULE_UNLOAD:
                break;
        }
}

//   HALF a test of the selection mechanism, and the safe half.
//
//   what it exercises: that a register WRITE reaches this chip at all, that SRC_PDO_SEL is
// where we think it is, and that the value our code builds for "profile 0" lands as written.
// That is the part most likely to be wrong in software, and none of it moves a rail.
//
//   what it deliberately does NOT do: write GO_COMMAND. SRC_PDO_SEL only HOLDS a selection --
// the request happens when GO_COMMAND is written, and nothing before that point renegotiates
// anything. So this cannot change the output voltage even if every assumption below is wrong.
//
//   why that matters here: the code this writes, 1 in the high nibble, is believed to mean 5V
// because PD_STATUS0 uses that numbering -- confirmed, since the chip reported 0x13 while the
// rail was measurably at 5V. That SRC_PDO_SEL shares the numbering is an inference by ANALOGY
// from third-party sources and is not confirmed. If it is off by one, "requesting 5V" requests
// 9V, and on this bench 9V reaches a Pico's 5V input. That is why the request half of the
// mechanism waits for a rail with nothing precious on it, and why this half is worth having
// separately: it removes every software doubt that can be removed without taking that risk.
//
//   NOT called from the sweep, and not called from anywhere. Invoke it deliberately:
//     scripts/debug.ps1 -Target screentest_husb238 -Batch -NoBuild -Attach \
//         -Ex 'call _test_pdo_sel_writeback()','print _sel_after','print _sel_status0_after'
//   non-static so it survives as a callable symbol
void _test_pdo_sel_writeback(void)
{
        //   the 5V code as light_power_husb238 would build it for profile 0, written out here
        // rather than reusing the driver's macro: this test is checking that construction, and
        // a test that borrows the thing it is checking proves nothing
        const uint8_t five_volt_sel = 1 << HUSB238_PDO_SEL_SHIFT;

        _sel_test_ran = 0;
        if(!_read_reg(HUSB238_REG_SRC_PDO_SEL, &_sel_before)
                        || !_read_reg(HUSB238_REG_PD_STATUS0, &_sel_status0_before)) {
                light_warn("sel test: chip not answering; nothing attempted");
                return;
        }
        light_info("sel test: SRC_PDO_SEL=%02x PD_STATUS0=%02x before",
                        _sel_before, _sel_status0_before);

        if(!light_ioport_write_register_byte(_io, HUSB238_REG_SRC_PDO_SEL, five_volt_sel)) {
                light_warn("sel test: write to SRC_PDO_SEL failed -- nothing changed");
                return;
        }
        (void)_read_reg(HUSB238_REG_SRC_PDO_SEL, &_sel_after);
        (void)_read_reg(HUSB238_REG_PD_STATUS0, &_sel_status0_after);
        light_info("sel test: wrote %02x, reads back %02x, PD_STATUS0=%02x",
                        five_volt_sel, _sel_after, _sel_status0_after);

        //   restored unconditionally, including after a surprising read-back: leaving a
        // selection staged is how a LATER GO_COMMAND -- from any source, including a
        // half-finished experiment -- would act on a choice nobody remembers making
        if(!light_ioport_write_register_byte(_io, HUSB238_REG_SRC_PDO_SEL, _sel_before))
                light_warn("sel test: could NOT restore SRC_PDO_SEL to %02x -- a selection is"
                                " left staged; do not write GO_COMMAND", _sel_before);
        (void)_read_reg(HUSB238_REG_SRC_PDO_SEL, &_sel_restored);

        //   the actual verdict. PD_STATUS0 changing would mean the write renegotiated
        // something on its own, which would falsify the premise this whole test rests on
        if(_sel_status0_after != _sel_status0_before)
                light_warn("sel test: PD_STATUS0 MOVED %02x -> %02x. Writing SRC_PDO_SEL is not"
                                " inert on this part -- stop and re-think before any GO_COMMAND",
                                _sel_status0_before, _sel_status0_after);
        else if(_sel_after != five_volt_sel)
                light_warn("sel test: wrote %02x but read back %02x -- the register does not"
                                " hold what we put in it", five_volt_sel, _sel_after);
        else
                light_info("sel test: write path good, rail untouched, restored to %02x",
                                _sel_restored);
        _sel_test_ran = 1;
}

static bool _poll_now(void);

//   the light_power view of the same chip, captured beside the raw registers so the two can
// be compared in one place. Forces a read rather than waiting for the module task's next
// throttled poll, so the snapshot matches the register dump taken moments earlier rather than
// something up to half a second older
static void _capture_power_view(void)
{
        if(!_pd) {
                light_warn("no light_power device -- driver did not come up");
                return;
        }
        bool read = _poll_now();

        _pd_is_pd = light_power_is_pd(_pd) ? 1 : 0;
        _pd_profile_count = light_power_profile_count(_pd);
        for(uint8_t i = 0; i < LIGHT_POWER_MAX_PROFILES; i++)
                light_power_get_profile(_pd, i, &_pd_profiles[i]);
        _pd_contract = light_power_get_active(_pd, &_pd_active_mv, &_pd_active_ma) ? 1 : 0;
        //   asked for 12000mV deliberately, against a device capped at 5000mV: the answer
        // proves the ceiling binds the FINDER as well as the selector. If this ever comes back
        // naming the 12V profile, find-then-select has become a trap that hands out an index
        // the next call refuses
        _pd_best_for_12v = light_power_find_profile(_pd, 12000);
        _pd_max_mv = light_power_get_max_millivolts(_pd);

        light_info("light_power: %s device, %d profiles, poll %s",
                        _pd_is_pd ? "PD-compliant" : "non-PD",
                        _pd_profile_count, read ? "ok" : "FAILED");
        for(uint8_t i = 0; i < _pd_profile_count; i++)
                light_info("  [%d] %5dmV %5dmA %s", i,
                                _pd_profiles[i].millivolts, _pd_profiles[i].milliamps,
                                _pd_profiles[i].available ? "offered" : "--");
        if(_pd_contract)
                light_info("  contract: %dmV %dmA", _pd_active_mv, _pd_active_ma);
        else
                light_info("  contract: none negotiated (rail is at the bus default)");
        light_info("  best profile at or below 12000mV: %d", _pd_best_for_12v);
}

//   the sweep runs ONCE, automatically, and that is what makes this rig usable on a board
// reached only over SWD. The interactive console is a USB CDC device served by the target's own
// USB port (see LIGHT_PLATFORM_USB_ON_CORE1), so a Pico wired to a debug probe and nothing else
// has no console at all and _poll_console() below compiles to nothing. Its log still reaches
// the host, over UART through the probe -- so the rig prints what it found rather than waiting
// to be asked, and the one question worth answering gets answered either way.
//   once, not on a timer: an unidentified chip should be read deliberately and then left alone.
static void _service_autosweep(uint32_t now)
{
        if(_swept || now - _boot_ms < HUSB238_AUTOSWEEP_DELAY_MS)
                return;
        _swept = true;

        light_info("--- automatic sweep ---");
        //   before anything transacts, so it measures the bus at rest rather than whatever a
        // failed transfer left behind
        _check_bus_idle();
        if(!_sweep_addresses())
                return;
        _dump_registers();
        _read_status();
        _capture_power_view();
        light_info("--- sweep complete ---");
}

// forces a read regardless of the module task's throttle -- see _capture_power_view()
static bool _poll_now(void)
{
        uint16_t saved = _pd->poll_interval_ms;
        light_power_set_poll_interval(_pd, 0);
        bool read = light_power_command_poll(_pd);
        light_power_set_poll_interval(_pd, saved);
        return read;
}

#if HUSB238_ALLOW_NEGOTIATION_TEST
//   walks every voltage the source offers, requesting each in turn and recording what actually
// appears on the rail afterwards. Run as a state machine across ticks rather than as a loop,
// because each step has to WAIT for a negotiation that completes in its own time, and blocking
// the scheduler for the better part of a second per profile is exactly the pattern this
// codebase has been burned by before.
//
//   what it is actually for: the driver builds a selection code by adding one to the profile
// index, on the belief that SRC_PDO_SEL numbers voltages the way PD_STATUS0 does. That belief
// is an inference by analogy and has never been checked. Asking for every voltage and reading
// back what arrives checks it directly -- and a table where requested and actual disagree by a
// consistent step is an off-by-one shown rather than argued about
static void _service_negotiation(uint32_t now)
{
        if(!_swept || _neg_done || !_pd)
                return;

        switch(_neg_phase) {
        case 0:
                //   the ceiling exists to stop exactly what this function does, so raising it
                // is the deliberate act the API asks for -- and it is logged as a warning by
                // light_power_set_max_millivolts(), which is correct: someone should be able
                // to find this in a log later and see who claimed the rail could take it
                light_info("--- negotiation test: output must be disconnected ---");
                light_power_set_max_millivolts(_pd, 20000);
                _neg_stage = 0;
                _neg_phase = 1;
                return;

        case 1: {
                //   skip what the source does not offer. Requesting an unavailable profile is
                // refused by light_power_select_profile() anyway, but stepping over it here
                // keeps the results table aligned with the profile list
                while(_neg_stage < light_power_profile_count(_pd)) {
                        struct power_profile p;
                        light_power_get_profile(_pd, _neg_stage, &p);
                        if(p.available)
                                break;
                        _neg_stage++;
                }
                if(_neg_stage >= light_power_profile_count(_pd)) {
                        //   finish at the bottom of the range rather than wherever the sweep
                        // ended. Leaving 20V standing on a bench rail because the loop
                        // happened to stop there is how a later "it was only 5V, surely"
                        // turns into a dead board
                        light_info("negotiation test: returning the rail to 5V");
                        light_power_select_profile(_pd, 0);
                        light_power_set_max_millivolts(_pd, 5000);
                        _neg_done = 1;
                        _neg_phase = 0;
                        return;
                }
                _neg_sent[_neg_stage] = light_power_select_profile(_pd, _neg_stage) ? 1 : 0;
                _neg_started_ms = now;
                _neg_phase = 2;
                return;
        }

        case 2:
                if(now - _neg_started_ms < NEG_SETTLE_MS)
                        return;
                _poll_now();
                _neg_result_contract[_neg_stage] =
                        light_power_get_active(_pd, &_neg_result_mv[_neg_stage],
                                                &_neg_result_ma[_neg_stage]) ? 1 : 0;
                //   the raw registers alongside the interpreted view, read through the app's
                // own io_context so the driver's state cannot colour what is reported
                (void)_read_reg(HUSB238_REG_SRC_PDO_SEL, &_neg_sel_after[_neg_stage]);
                (void)_read_reg(HUSB238_REG_PD_STATUS0, &_neg_status0_after[_neg_stage]);
                //   PD_STATUS1 carries a PD_RESPONSE field, which is the source's own verdict
                // on the request. That is the difference between "we asked and were refused"
                // and "we never really asked" -- and PD_STATUS0 alone cannot tell them apart,
                // because a refused request simply leaves the previous contract standing
                (void)_read_reg(HUSB238_REG_PD_STATUS1, &_neg_status1_after[_neg_stage]);
                _neg_state[_neg_stage] = light_power_request_state(_pd);
                light_info("  stage %d: SRC_PDO_SEL=%02x PD_STATUS0=%02x PD_STATUS1=%02x",
                                _neg_stage, _neg_sel_after[_neg_stage],
                                _neg_status0_after[_neg_stage], _neg_status1_after[_neg_stage]);
                {
                        struct power_profile p;
                        light_power_get_profile(_pd, _neg_stage, &p);
                        light_info("  asked %5dmV -> got %5dmV %5dmA %s", p.millivolts,
                                        _neg_result_mv[_neg_stage], _neg_result_ma[_neg_stage],
                                        _neg_result_contract[_neg_stage] ? "(contract)" : "(no contract)");
                        if(_neg_result_contract[_neg_stage]
                                        && _neg_result_mv[_neg_stage] != p.millivolts)
                                light_warn("    MISMATCH -- the selection encoding is wrong");
                }
                _neg_stage++;
                _neg_phase = 1;
                return;
        }
}
#else
static void _service_negotiation(uint32_t now) { (void)now; }
#endif

static uint8_t _husb238_main(struct light_application *app)
{
        uint32_t now = light_platform_get_time_since_init();
        _service_autosweep(now);
        _service_negotiation(now);
        // a no-op unless this build has a USB console; see _poll_console()
        _poll_console();
        return LF_STATUS_RUN;
}
