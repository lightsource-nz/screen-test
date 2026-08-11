#include <screentest_hw_ws_touch169.h>
#include <light_backlight.h>
#include <light_display_st7789.h>
#include <light_imu_qmi8658.h>
#include <light_touch_cst816t.h>

// board: Waveshare RP2350-Touch-LCD-1.69 -- ST7789 panel (240x280, 16bpp) plus a CST816T
// capacitive touch controller. moved here verbatim from screentest_ws_touch169's app.c so
// the UI demo app can build against the same board without copying it

// no direct hardware includes any more: the backlight was the last thing this file drove as
// a raw GPIO, and light_backlight owns that pin now

struct display_device *screentest_hw_ws_touch169_display(void)
{
        struct io_context *io = light_ioport_setup_io_spi_4p(
                PORT_SPI_1,
                ST_DISPLAY_PIN_RESET, ST_DISPLAY_PIN_CS, ST_DISPLAY_PIN_DC,
                ST_DISPLAY_PIN_SCK, ST_DISPLAY_PIN_MOSI);
        // before the device is created, so even the initialisation sequence runs at the
        // faster clock -- if the panel cannot take it, it fails visibly from the first
        // frame rather than only once something animates
        light_ioport_set_spi_clock(io, ST_DISPLAY_SPI_HZ);
        struct display_device *disp = light_display_st7789_create_device(
                "screentest_display_main", ST_DISPLAY_WIDTH, ST_DISPLAY_HEIGHT, io);

        // both row_offset=20 and col_offset=20 were tried here and had zero visible effect
        // on a persistent noise strip -- but that test was confounded by screentest_common's
        // render context being hardcoded to the small OLED test rigs' 64x128 1bpp geometry
        // (since fixed, see ST_RENDER_WIDTH/HEIGHT/BPP in the app's screentest.h) and by a
        // width/height axis experiment (also since reverted). now that both are fixed and
        // the noise strip is confirmed to persist against a genuinely correct buffer,
        // retrying the original row_offset=20 guess -- 20 is the commonly-cited GDDRAM
        // offset for this exact panel size in the maker community
        light_display_st7789_set_offset(disp, 0, 20);
        // creating the device already cleared the panel -- but that ran with the offset
        // still at its default of 0, so it blanked GDDRAM rows 0..279 while every update
        // from here on writes rows 20..299. that leaves rows 280..299 holding whatever
        // powered up in them. a full-frame update happens to paint over that band every
        // frame, which is why it stayed hidden until updates became region-limited and
        // stopped touching it. clear again now that the offset is right
        light_display_command_clear(disp, 0);

        light_info("display pipeline setup complete","");
        return disp;
}

struct touch_device *screentest_hw_ws_touch169_touch(void)
{
        // shared I2C1 bus (also used by IMU/RTC, not yet implemented) -- setup_io_i2c
        // re-inits the same peripheral each time it's called, harmless as long as the
        // params (scl/sda) agree, which they will once IMU/RTC support lands
        struct io_context *io = light_ioport_setup_io_i2c(
                PORT_I2C_1, ST_TOUCH_PIN_RST, CST816T_I2C_ADDR,
                ST_TOUCH_PIN_SCL, ST_TOUCH_PIN_SDA);
        struct touch_device *touch = light_touch_cst816t_create_device(
                "screentest_touch_main", ST_TOUCH_X_MAX, ST_TOUCH_Y_MAX, io, ST_TOUCH_PIN_INT);

        light_info("touch pipeline setup complete","");
        return touch;
}

struct backlight_device *screentest_hw_ws_touch169_backlight(void)
{
        // the backlight is a plain pin rather than anything the ST7789 knows about, which is
        // why it was driven straight from here as an on/off GPIO before light_backlight
        // existed. active high: this board's enable line sources into the LED driver
        return light_backlight_pwm_create_device(
                "screentest_backlight_main", ST_DISPLAY_PIN_BL, false);
}

struct imu_device *screentest_hw_ws_touch169_imu(void)
{
        // its own io_context despite sharing the bus with the touch controller: an
        // io_context carries the target's I2C address, and these two answer to different
        // ones. setup_io_i2c re-inits the same peripheral, which is harmless as long as the
        // scl/sda parameters agree -- and they do, being the same two pins.
        //
        // no reset line: the IMU has none of its own on this board, and pulsing the touch
        // controller's out from under it would be actively wrong, hence PIN_NONE
        struct io_context *io = light_ioport_setup_io_i2c(
                PORT_I2C_1, LIGHT_IOPORT_PIN_NONE, QMI8658_I2C_ADDR,
                ST_TOUCH_PIN_SCL, ST_TOUCH_PIN_SDA);
        struct imu_device *imu = light_imu_qmi8658_create_device(
                "screentest_imu_main", io, ST_IMU_PIN_INT1);

        // declare the mounting before anything reads a sample, so orientation is classified
        // in the device frame from the very first poll rather than settling wrong and then
        // being corrected
        light_imu_set_axis_map(imu, ST_IMU_AXIS_MAP);

        light_info("imu pipeline setup complete","");
        return imu;
}
