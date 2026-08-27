#include <light.h>
#if(LIGHT_SYSTEM == SYSTEM_PICO_SDK)
// for gpio_put()/gpio_set_function() -- see the pin identification mode below
#include <hardware/gpio.h>
#endif
#include <light_platform.h>
#include <light_display_ssd1322.h>
#include <module/mod_light_display.h>
#include <module/mod_light_display_ssd1322.h>
// light_draw declares its own module symbol in light_draw.h rather than under module/, unlike
// every other module here -- it arrives via light_display.h, so there is nothing to include

// app: screentest_ssd1322
//   bring-up rig for a 256x64 SSD1322 greyscale OLED on a plain Pico 2, over 4-wire SPI.
//
//   the panel is driven from a ONE-BIT-PER-PIXEL render context: light_draw has no 4bpp path
// and the driver expands each mono pixel to 0x0 or 0xF on its way out. See
// light_display_ssd1322.h for why that trade was made rather than adding a third pixel depth
// to a module every display depends on.

#define ST_SSD1322_WIDTH                256
#define ST_SSD1322_HEIGHT               64
//   1bpp, matching what the driver expects. Creating this at 16 would allocate eight times
// the buffer and hand the driver pixels it does not know how to read
#define ST_SSD1322_BPP                  1

//   the wiring, and the one place it is written down. CS, D/C and RES are plain GPIOs --
// light_ioport drives CS as SIO rather than as the SPI peripheral's own chip-select -- so only
// SCK and MOSI are constrained, and GP2/GP3 are SPI0's pins on this chip.
//
//   CHOSEN AROUND WHAT THIS BENCH ALREADY HAS ON IT, which is why they are not the obvious
// GP16-GP20. That rig carries a Pico-OLED-1.3 and a second SH1107, and light_ui_hw_po13.h
// claims GP16/17/18/19/20 for the second panel and GP15/GP17 for the keys -- exactly the five
// pins an SPI display wants by default. Also spoken for: GP6-GP12 by the Pico-OLED-1.3 itself
// over spi1, GP4/GP5 by the HUSB238 probe's I2C, and GP0/GP1 by the debug probe's UART.
//   what is left, and what these are: GP2/GP3 for the bus, GP13/GP14/GP21 for control.
#define ST_SSD1322_PORT_ID              PORT_SPI_0
#define ST_SSD1322_PIN_DC               14      // module pin 4,  header pin 19
#define ST_SSD1322_PIN_CS               13      // module pin 16, header pin 17
#define ST_SSD1322_PIN_SCK               2      // module pin 7  (D0), header pin 4
#define ST_SSD1322_PIN_MOSI              3      // module pin 8  (D1), header pin 5
#define ST_SSD1322_PIN_RESET            21      // module pin 15, header pin 27

//   how often the test pattern is redrawn and pushed. Slow on purpose: this rig exists to be
// looked at, and a panel updating at frame rate tells you nothing more than one updating twice
// a second while making a stuck update harder to notice
#define ST_SSD1322_UPDATE_INTERVAL_MS   500

//   LAMP TEST. Alternates SSD1322 "entire display on" (0xA5, every pixel lit from the
// controller, GDDRAM ignored) with the normal RAM-driven test pattern, a few seconds each.
//   this is the measurement that splits a dark panel in two. Under 0xA5 the high rail, VSL,
// VCOMH, contrast and the panel itself are all exercised with NO dependence on RAM contents,
// the column offset, the row window or any pixel data having arrived. So: lit under 0xA5 and
// dark under the pattern means the fault is in the data path; dark under both means it is in
// power or init, and those want completely different next steps
#define ST_SSD1322_LAMP_TEST            0
#define ST_SSD1322_LAMP_PHASE_MS        4000

//   PIN IDENTIFICATION MODE. Set to 1 to abandon the display entirely and instead walk the
// five signal lines one at a time, holding exactly one LOW while the other four sit HIGH, for
// long enough to find it with a voltmeter.
//
//   this exists because the module's 16-pin header is unlabelled and our mapping of it rests
// on an inference -- pin 3 reading open-circuit -- rather than on a datasheet. Everything
// upstream of the connector has now been verified twice over, by SWD register reads and by the
// target's own boot log, so a wrong mapping is the remaining way for all of that to be true
// and the panel still dark.
//   and it identifies rather than merely checks: whichever header pin reads ~0V while the
// console announces a signal is that signal, whatever we assumed. If the answers disagree with
// the table in this file, the table is wrong and the header's real order is what you just read.
#define ST_SSD1322_PIN_IDENTIFY         0
// long enough to move a probe and read it without hurrying
#define ST_SSD1322_IDENTIFY_STEP_MS     2000

static struct light_draw_context *_render;
static struct display_device *_display;
static uint32_t _next_update_ms;
static uint32_t _frame;
#if ST_SSD1322_LAMP_TEST
// which quarter of the lamp cycle is in force, so commands are sent only on a change. Starts
// at an impossible value so the first tick always applies a phase rather than assuming one
static uint32_t _lamp_phase = 0xFFFFFFFF;
#endif

static void _ssd1322_event(const struct light_module *module, uint8_t event, void *arg);
static uint8_t _ssd1322_main(struct light_application *app);

Light_Application_Define(screentest_ssd1322, _ssd1322_event, _ssd1322_main,
                                &light_draw,
                                &light_display,
                                &light_display_ssd1322,
                                &light_core);

void main(int argc, char **argv)
{
        light_framework_init();
        // (0, NULL) rather than (argc, argv): nothing sets those on a bare-metal entry point
        light_framework_run(0, NULL);
}

//   THE TEST PATTERN, and every element of it is chosen to fail visibly rather than
// plausibly. The SSD1322's failure mode on this bench is not a dark panel -- it is a picture
// that is present but wrong, because each column address covers four pixels and a wrong
// offset shifts the image and wraps it round rather than blanking it.
//
//   so: a border hard against all four edges, which is the offset check. If the left edge is
// missing, or a stripe of it appears at the right, the column offset is wrong -- and the
// amount of wrap says by how many groups of four.
//   a full-height diagonal, which is the row-addressing and stride check. Any error in bytes
// per row makes a straight line into a staircase or a shear.
//   and a corner block, which breaks the symmetry: a border and a diagonal alone look
// identical under a 180-degree remap, and this says which way up the panel actually is.
static void _draw_test_pattern(void)
{
        light_draw_draw_clear(_render);

        // the border: last drawable pixel on each axis, so any offset error eats an edge
        light_draw_draw_rect(_render,
                        (light_draw_point2d){ 0, 0 },
                        (light_draw_point2d){ ST_SSD1322_WIDTH - 1, ST_SSD1322_HEIGHT - 1 },
                        false);

        // corner-to-corner, so a stride error shears it away from the corners it should touch
        light_draw_draw_line(_render,
                        (light_draw_point2d){ 0, 0 },
                        (light_draw_point2d){ ST_SSD1322_WIDTH - 1, ST_SSD1322_HEIGHT - 1 },
                        true);

        //   the orientation marker, in the top-left. Deliberately not centred and not
        // symmetric: it is the only element here that distinguishes an upright panel from one
        // that is upside down, which the remap byte can silently make it
        light_draw_draw_rect(_render,
                        (light_draw_point2d){ 4, 4 },
                        (light_draw_point2d){ 19, 11 },
                        true);

        //   a moving marker, so a frozen update is distinguishable from a correct static one.
        // Without it a panel that renders once and then wedges looks exactly like a panel that
        // is working, which is a bad thing to be unable to tell apart during bring-up
        uint16_t x = (uint16_t)(24 + (_frame % 16) * 4);
        light_draw_draw_rect(_render,
                        (light_draw_point2d){ x, 4 },
                        (light_draw_point2d){ (uint16_t)(x + 2), 6 },
                        true);
}

static void _ssd1322_event(const struct light_module *module, uint8_t event, void *arg)
{
        switch(event) {
        case LF_EVENT_MODULE_LOAD:;
                _render = light_draw_context_create("screentest_ssd1322_render",
                                ST_SSD1322_WIDTH, ST_SSD1322_HEIGHT, ST_SSD1322_BPP);

                struct io_context *io = light_ioport_setup_io_spi_4p(
                                ST_SSD1322_PORT_ID, ST_SSD1322_PIN_RESET, ST_SSD1322_PIN_CS,
                                ST_SSD1322_PIN_DC, ST_SSD1322_PIN_SCK, ST_SSD1322_PIN_MOSI);

                //   SLOWED DELIBERATELY, and before the device is created so the very first
                // init byte goes out at this rate. light_ioport defaults to 10MHz, which lands
                // at 9.375MHz after the divider -- and the SSD1322's serial interface is
                // specified to about 10MHz (100ns cycle). That is not headroom, it is the
                // ceiling, on flying leads with no ground plane.
                //   marginal SPI does not fail by going quiet; it fails by delivering corrupted
                // bytes, which presents as a controller that ignores everything -- exactly what
                // a dark panel under 0xA5 looks like. 1MHz is far enough back that signal
                // integrity cannot be the explanation, and a full frame still takes only ~65ms
                uint32_t rate = light_ioport_set_spi_clock(io, 1000000);
                light_info("spi clock set to %u Hz for bring-up (was ~9.375MHz)", (unsigned)rate);

                _display = light_display_ssd1322_create_device("screentest_ssd1322_main",
                                ST_SSD1322_WIDTH, ST_SSD1322_HEIGHT, io);
                light_display_set_render_context(_display, _render);

                light_info("ssd1322 pipeline ready: %dx%d @ %dbpp on spi port %d",
                                ST_SSD1322_WIDTH, ST_SSD1322_HEIGHT, ST_SSD1322_BPP,
                                ST_SSD1322_PORT_ID);
                _next_update_ms = 0;
                _frame = 0;
                break;
        case LF_EVENT_MODULE_UNLOAD:
                break;
        }
}

#if ST_SSD1322_PIN_IDENTIFY
//   the five lines, in the order they are walked. Named alongside the module pin this file
// BELIEVES each one lands on, so the console announcement can be compared directly against
// what the meter finds -- that comparison is the entire point
static const struct { uint8_t gpio; const char *signal; uint8_t believed_module_pin; } _id_pins[] = {
        { ST_SSD1322_PIN_RESET, "RES",  15 },
        { ST_SSD1322_PIN_CS,    "CS",   16 },
        { ST_SSD1322_PIN_DC,    "D/C",   4 },
        { ST_SSD1322_PIN_SCK,   "SCLK",  7 },
        { ST_SSD1322_PIN_MOSI,  "MOSI",  8 },
};
#define ID_PIN_COUNT (sizeof(_id_pins) / sizeof(_id_pins[0]))

static uint8_t _id_step;
static uint32_t _id_next_ms;
static bool _id_setup_done;

//   takes all five away from their peripherals and drives them as plain outputs. SCLK and MOSI
// are on SPI0 in normal operation, so they must be reclaimed as SIO or they will not follow
// anything written here
static void _identify_setup(void)
{
#if(LIGHT_SYSTEM == SYSTEM_PICO_SDK)
        for(uint8_t i = 0; i < ID_PIN_COUNT; i++) {
                gpio_set_function(_id_pins[i].gpio, GPIO_FUNC_SIO);
                gpio_set_dir(_id_pins[i].gpio, true);
                gpio_put(_id_pins[i].gpio, true);
        }
#endif
        light_info("--- pin identification: one line LOW at a time, %dms each ---",
                        ST_SSD1322_IDENTIFY_STEP_MS);
        light_info("meter each module header pin against ground; the pin reading ~0V is the");
        light_info("signal named below, whatever this rig believes it is wired to");
        _id_setup_done = true;
}
static void _service_identify(uint32_t now)
{
        if(!_id_setup_done)
                _identify_setup();
        if(now < _id_next_ms)
                return;
        _id_next_ms = now + ST_SSD1322_IDENTIFY_STEP_MS;

#if(LIGHT_SYSTEM == SYSTEM_PICO_SDK)
        // everything high, then exactly one low -- so a meter sees one unambiguous winner
        for(uint8_t i = 0; i < ID_PIN_COUNT; i++)
                gpio_put(_id_pins[i].gpio, true);
        gpio_put(_id_pins[_id_step].gpio, false);
#endif
        //   COMMENTED OUT because it fires every step forever and floods a console that this
        // board drains on core 0. Uncomment when actually using this mode -- the announcement
        // IS the mode's output, and without it you are metering pins with no idea which signal
        // is currently low
        // light_info("  NOW LOW: %-4s  (gp%d, believed to be module pin %d)",
        //                 _id_pins[_id_step].signal, _id_pins[_id_step].gpio,
        //                 _id_pins[_id_step].believed_module_pin);
        _id_step = (uint8_t)((_id_step + 1) % ID_PIN_COUNT);
}
#endif

static uint8_t _ssd1322_main(struct light_application *app)
{
        uint32_t now = light_platform_get_time_since_init();

#if ST_SSD1322_PIN_IDENTIFY
        //   the display is not driven at all in this mode: the SPI pins have been reclaimed as
        // GPIOs, so anything the driver sent would go nowhere and the transfers would simply
        // hang the update path
        _service_identify(now);
        return LF_STATUS_RUN;
#endif

        if(!_display || now < _next_update_ms)
                return LF_STATUS_RUN;
        _next_update_ms = now + ST_SSD1322_UPDATE_INTERVAL_MS;

#if ST_SSD1322_LAMP_TEST
        //   FOUR phases, not two: both VSL selections crossed with both display modes, so one
        // flash tests every combination rather than needing a rebuild between them. VSL is the
        // init value most likely to be wrong on an unfamiliar module, and getting it backwards
        // gives a dark panel with everything else correct -- which is precisely what we have
        uint32_t phase = (now / ST_SSD1322_LAMP_PHASE_MS) % 4;
        if(phase != _lamp_phase) {
                _lamp_phase = phase;
                bool internal_vsl = phase >= 2;
                bool lamp = (phase % 2) == 0;

                //   VSL first, then the display mode: the enhancement command reconfigures
                // segment drive, and setting it after 0xA5 would briefly light the panel under
                // the OLD setting -- which is the thing being tested
                light_display_ssd1322_set_vsl(_display, internal_vsl
                                ? SSD1322_ENHANCE_A_VSL_INTERNAL : SSD1322_ENHANCE_A_VSL_EXTERNAL);
                light_display_ssd1322_command_set_entire_on(_display, lamp);

                //   COMMENTED OUT for the same reason as the pin walk's: it repeats forever.
                // Uncomment alongside enabling the mode, since knowing which of the four
                // combinations is on screen is the whole point of the sweep
                // light_info("lamp test: VSL=%s (0x%02x), %s",
                //                 internal_vsl ? "INTERNAL" : "EXTERNAL",
                //                 internal_vsl ? SSD1322_ENHANCE_A_VSL_INTERNAL
                //                              : SSD1322_ENHANCE_A_VSL_EXTERNAL,
                //                 lamp ? "ENTIRE ON (0xA5)" : "NORMAL (0xA6), RAM pattern");
        }
        //   RAM keeps being updated through every phase. 0xA5 overrides what is displayed
        // without disturbing GDDRAM, so the pattern is already correct underneath when a
        // normal phase returns -- no re-push, and no blank frame at the transition
#endif

        _draw_test_pattern();
        _frame++;
        //   the whole panel every time, deliberately: region tracking is an optimisation and
        // this rig is checking that the addressing works at all. A partial update that looked
        // right would prove less than a full one that does
        light_display_command_update(_display);

        return LF_STATUS_RUN;
}
