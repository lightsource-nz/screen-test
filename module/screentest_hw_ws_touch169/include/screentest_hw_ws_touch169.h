#ifndef _SCREENTEST_HW_WS_TOUCH169_H
#define _SCREENTEST_HW_WS_TOUCH169_H

#include <light_display.h>
#include <light_imu.h>
#include <light_touch.h>

// board wiring and device construction for the Waveshare RP2350-Touch-LCD-1.69, factored
// out of the demo application that used to own it so more than one app can be built
// against the same board without restating (or drifting from) its pinout

// ST7789 panel, SPI 4-wire on real spi1 hardware -- unlike crossfire's PIO-emulated SPI,
// this board has nothing else contending for spi1. all confirmed from the board's
// schematic + wiki pin table during bring-up
#define ST_DISPLAY_PIN_DC               8
#define ST_DISPLAY_PIN_CS               9
#define ST_DISPLAY_PIN_SCK              10
#define ST_DISPLAY_PIN_MOSI             11
#define ST_DISPLAY_PIN_RESET            13
#define ST_DISPLAY_PIN_BL               25

// tried width/height swapped (280x240) as an experiment to explain a noise strip seen
// during bring-up -- disproved, not confirmed: it made the noise strip wider and fragmented
// the test circle into a horizontally-repeating row of smaller circles (a column-address-
// wraparound signature -- 280 exceeds this panel's real native column capacity). back to
// the product spec's 240x280, which the evidence now says was correct all along
#define ST_DISPLAY_WIDTH                240
#define ST_DISPLAY_HEIGHT               280

// CST816T touch controller -- shared I2C1 bus (also used by IMU/RTC, not yet implemented),
// confirmed from the board's schematic + wiki pin table during bring-up
#define ST_TOUCH_PIN_SDA                6
#define ST_TOUCH_PIN_SCL                7
#define ST_TOUCH_PIN_INT                21
#define ST_TOUCH_PIN_RST                22
#define ST_TOUCH_X_MAX                  ST_DISPLAY_WIDTH
#define ST_TOUCH_Y_MAX                  ST_DISPLAY_HEIGHT

// QMI8658C 6-axis IMU -- same shared I2C1 bus as the touch controller, at its own address.
// INT1/INT2 confirmed from the board's schematic + wiki pin table; INT1 is passed to the
// driver but not yet used to gate sampling (see light_imu_qmi8658.h)
#define ST_IMU_PIN_INT1                 23
#define ST_IMU_PIN_INT2                 24

// how this board mounts the QMI8658C, as the axis map light_imu rotates every sample
// through (see struct imu_axis_map). a board-MOUNTING fact rather than a driver one, so it
// lives here where a single edit corrects it -- the same treatment the display's row offset
// needed. declaring it once means BOTH orientation reporting and tilt steering come out
// right, instead of each compensating separately.
//
// CONFIRMED ON HARDWARE, using the convention that an accelerometer axis pointing UP reads
// +1g. three observations fix all three axes:
//   - lowering the right edge drove the circle DOWN, so the chip's +Y points right
//   - lowering the bottom edge drove it RIGHT, so the chip's +X points up the screen
//   - face-up was reported as face-down, so the chip's +Z points INTO the screen
// the X/Y transposition and the Z inversion corroborate each other: transposing two axes
// alone would flip handedness, which no physical mounting can do, and negating the third
// restores it. so this is a real rotation, not two independent guesses that happen to fit
#define ST_IMU_AXIS_MAP \
        ((struct imu_axis_map) { \
                .source = { IMU_AXIS_Y, IMU_AXIS_X, IMU_AXIS_Z }, \
                .sign = { 1, 1, -1 } \
        })

extern struct display_device *screentest_hw_ws_touch169_display(void);
extern struct touch_device *screentest_hw_ws_touch169_touch(void);
extern struct imu_device *screentest_hw_ws_touch169_imu(void);

#endif
