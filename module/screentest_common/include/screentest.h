#ifndef _SCREENTEST_H
#define _SCREENTEST_H

#include <light.h>
#include <light_display.h>
#include <light_touch.h>

#include <stdint.h>

// TODO implement version fields properly
#define ST_VERSION_STR                  "0.1.0"

#define ST_INFO_STR                     "screen-test v" CF_VERSION_STR

// TODO make display count configurable at runtime
#define ST_DISPLAY_COUNT                2

// default render context geometry -- matches this file's own hardcoded values before
// they became overridable, i.e. the small portrait OLED test rigs (screentest_po13,
// which has no screentest.h of its own and falls back to this file). apps with a
// differently-shaped display (e.g. screentest_ws_touch169's 240x280 16bpp panel)
// provide their own complete screentest.h that shadows this one entirely, including
// these four -- see light_ioport's PORT_SPI_1 wiring convention for why that
// shadowing already has to work this way for ST_DISPLAY_COUNT et al
#define ST_RENDER_WIDTH                 64
#define ST_RENDER_HEIGHT                128
#define ST_RENDER_BPP                   1
#define ST_RENDER_ROTATION              REND_ROTATE_90
// roughly centered in the 128x64 logical (post-rotation) canvas the animated test circle
// draws against -- also overridable per-app, same rationale as the geometry above
#define ST_RENDER_CIRCLE_X              64
#define ST_RENDER_CIRCLE_Y              32

// how often the shared app redraws. also overridable per-app: a display that can't flush
// a frame this often will simply skip frames rather than fall behind, since the render
// loop already gates on light_display_render_context_busy()
#define ST_FRAME_RATE                   24
// the animated test circle grows from 1px to this radius, then restarts
#define ST_CIRCLE_MAX_RADIUS            16
// ...at this many pixels per second. expressed as a RATE rather than a per-frame step so
// the animation runs at the same speed whatever ST_FRAME_RATE is, and doesn't lurch when a
// frame gets skipped. with the two values above that's a 2 second cycle, stepping a pixel
// every 125ms -- slow enough to read as growth, fine enough not to look like it's jumping
#define ST_CIRCLE_GROWTH_PX_PER_S       8
// each swipe slides the circle this far in the swiped direction, over this long. the
// circle's centre wraps at the canvas edges, so it can be driven around indefinitely
#define ST_SWIPE_MOVE_DISTANCE          40
#define ST_SWIPE_MOVE_MS                400

extern struct display_device *_display[ST_DISPLAY_COUNT];
// NULL on boards with no touch hardware -- __screentest_hardware_init() only assigns
// this on the one board that has a touch device (screentest_ws_touch169); everywhere
// else it stays unset (zero-initialized), and the touch-triggered animation in app.c
// is a no-op whenever it's NULL
extern struct touch_device *_touch_main;

extern void __screentest_hardware_init();

#endif