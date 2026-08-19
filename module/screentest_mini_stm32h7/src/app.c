/*
 *  app.c
 *  screen-test on the WeAct MiniSTM32H7xx
 *
 *  The first thing to put pixels on this board's panel, and deliberately the simplest one that
 *  exercises the whole path: light_draw renders into a 160x80 16bpp buffer, light_canvas paces and
 *  flushes frames, light_display drives the ST7735S through light_ioport's new STM32 SPI
 *  transport.
 *
 *  What it draws is chosen to make the two failure modes of an ST7735 bring-up obvious rather
 *  than subtle. A one-pixel border at the very edge of the buffer shows immediately whether the
 *  GDDRAM offsets are right: if they are wrong the border is clipped on two sides and a strip
 *  of noise appears on the others. A block of solid colour shows whether the colour order is
 *  right, since a HannStar/BOE mismatch swaps red and blue.
 */
#include <light.h>
#include <light_platform.h>
#include <light_canvas.h>
#include <light_display.h>
#include <screentest_hw_mini_stm32h7.h>
#include <light_backlight.h>
#include <light_draw.h>

// the module descriptors named in Light_Application_Define() below. light_draw declares its own in
// light_draw.h; the rest live in a module/ subheader each
#include <module/mod_light_canvas.h>
#include <module/mod_light_display.h>
#include <module/mod_light_ioport.h>
#include <module/mod_light_display_st7735.h>
#include <module/mod_light_backlight.h>

#include <stm32h7xx.h>

static void app_event(const struct light_module *mod, uint8_t event, void *arg);
static uint8_t app_main(struct light_application *app);

Light_Application_Define(screentest_mini_stm32h7, app_event, app_main,
                                &light_canvas,
                                &light_display,
                                &light_ioport,
                                &light_display_st7735,
                                &light_backlight,
                                &light_draw,
                                &light_core);

#define FRAME_RATE              20
#define BLINK_INTERVAL_MS       500

// RGB565. Written out rather than via a helper because seeing the literal is what makes a
// colour-order fault readable: if RED shows as blue, the panel variant is wrong
#define RGB565(r, g, b)         ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
#define COLOR_BG                RGB565(0x00, 0x00, 0x00)
#define COLOR_BORDER            RGB565(0xFF, 0xFF, 0xFF)
#define COLOR_RED               RGB565(0xFF, 0x00, 0x00)

static struct display_device *_display;
static struct backlight_device *_backlight;
static struct canvas_context *_canvas;
static struct light_draw_context *_render;
static uint32_t _next_toggle_ms;
static bool _led_on;
static uint16_t _sweep_x;

//   a slow brightness ramp, purely so the PWM is visibly doing something a GPIO could not.
// Triangular rather than sawtooth: a jump from full to zero is a blink, which is exactly what
// a broken PWM looks like, so the ramp has to be continuous in both directions to be evidence
#define BACKLIGHT_STEP          12
static uint16_t _bl_level = LIGHT_BACKLIGHT_LEVEL_MAX;
static bool _bl_falling = true;

static void _led_write(bool on)
{
        // active low, as on light_board.h
        if(on)
                ST_LED_PORT->BSRR = (1U << (ST_LED_PIN + 16));
        else
                ST_LED_PORT->BSRR = (1U << ST_LED_PIN);
}

static void app_event(const struct light_module *mod, uint8_t event, void *arg)
{
        switch(event) {
        case LF_EVENT_MODULE_LOAD:
                RCC->AHB4ENR |= RCC_AHB4ENR_GPIOEEN;
                ST_LED_PORT->MODER &= ~(3U << (ST_LED_PIN * 2));
                ST_LED_PORT->MODER |= (1U << (ST_LED_PIN * 2));
                _led_write(false);

                _render = light_draw_context_create("screentest_render",
                                ST_DISPLAY_WIDTH, ST_DISPLAY_HEIGHT, ST_DISPLAY_BPP);

                _display = screentest_hw_mini_stm32h7_display();
                light_display_set_render_context(_display, _render);

                // after the display: the panel is cleared by then, so the first thing lit is a
                // black screen rather than whatever powered up in GDDRAM
                _backlight = screentest_hw_mini_stm32h7_backlight();

                _canvas = light_canvas_create(_render, &_display, 1);
                //   single-buffered on purpose for this first bring-up. A 160x80x16bpp buffer
                // is 25.6KB, so double buffering costs another 25.6KB of the 512KB AXI SRAM
                // and is affordable -- but it also hides a class of fault, where a frame looks
                // right because the previous one is still on the panel. Worth enabling once
                // the panel is known good.
                light_canvas_set_frame_rate(_canvas, FRAME_RATE);

                _next_toggle_ms = light_platform_get_time_since_init() + BLINK_INTERVAL_MS;
                light_info("screen-test up: %dx%d %dbpp on ST7735S",
                                ST_DISPLAY_WIDTH, ST_DISPLAY_HEIGHT, ST_DISPLAY_BPP);
                break;
        case LF_EVENT_MODULE_UNLOAD:
                break;
        }
}

static uint8_t app_main(struct light_application *app)
{
        uint32_t now = light_platform_get_time_since_init();

        // the LED is the "is anything running at all" signal, independent of the panel. If the
        // screen stays dark but this blinks, the fault is in the display path rather than in
        // the framework or the clock
        if((int32_t)(now - _next_toggle_ms) >= 0) {
                _led_on = !_led_on;
                _led_write(_led_on);
                _next_toggle_ms = now + BLINK_INTERVAL_MS;
        }

        //   DIAGNOSTIC, temporary -- see the matching block in the board module. With this set
        // the panel keeps whatever the init-time clear put there and nothing is drawn over it,
        // which is what makes "did the transport and init work" answerable separately from
        // "does the frame push work"
#if SCREENTEST_H7_DIAG_SOLID_FILL
        return LF_STATUS_RUN;
#endif

        if(light_canvas_frame_begin(_canvas)) {
                light_draw_draw_clear(_render);

                //   a border on the outermost pixel ring. This is the offset test: with the
                // GDDRAM offsets right it sits flush against all four edges of the glass, and
                // with them wrong it is clipped on two sides with noise on the others
                light_draw_draw_rect(_render,
                        (light_draw_point2d){ 0, 0 },
                        (light_draw_point2d){ ST_DISPLAY_WIDTH - 1, ST_DISPLAY_HEIGHT - 1 }, false);

                // a solid block that sweeps across, which both proves frames are actually
                // being pushed and gives a large area of known colour to check red/blue order
                uint16_t x = _sweep_x;
                light_draw_draw_rect(_render,
                        (light_draw_point2d){ (uint16_t)(x + 4), 20 },
                        (light_draw_point2d){ (uint16_t)(x + 28), 60 }, true);

                light_canvas_invalidate_all(_canvas);
                light_canvas_frame_end(_canvas);

                _sweep_x += 2;
                if(_sweep_x > ST_DISPLAY_WIDTH - 34)
                        _sweep_x = 0;

                //   ramp the backlight once per frame, so a full sweep takes a few seconds and
                // reads as a fade rather than a flicker. Bottoms out at a fifth rather than at
                // zero: a backlight that reaches 0 is indistinguishable from one that has
                // failed, which would make this a worse test than no test
                if(_backlight) {
                        if(_bl_falling) {
                                if(_bl_level <= LIGHT_BACKLIGHT_LEVEL_MAX / 5)
                                        _bl_falling = false;
                                else
                                        _bl_level -= BACKLIGHT_STEP;
                        } else {
                                if(_bl_level >= LIGHT_BACKLIGHT_LEVEL_MAX - BACKLIGHT_STEP)
                                        _bl_falling = true;
                                else
                                        _bl_level += BACKLIGHT_STEP;
                        }
                        light_backlight_set_level(_backlight, _bl_level);
                }
        }

        return LF_STATUS_RUN;
}

int main(void)
{
        light_framework_init();
        light_framework_run(0, NULL);
        return 0;
}
