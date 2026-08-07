#include <screentest.h>
#include <light_display_st7789.h>
#include <light_touch_cst816t.h>

void __screentest_hardware_init();

// app: screentest_ws_touch169
// board's ST7789 panel, 240x280, SPI 4-wire on GPIO8-11 (real spi1 hardware -- unlike
// crossfire's PIO-emulated SPI, this board has nothing else contending for spi1)

#if(LIGHT_SYSTEM == SYSTEM_PICO_SDK)
#include <hardware/gpio.h>
#endif

void __screentest_hardware_init()
{
        struct io_context *io = light_ioport_setup_io_spi_4p(
                PORT_SPI_1,
                ST_DISPLAY_PIN_RESET, ST_DISPLAY_PIN_CS, ST_DISPLAY_PIN_DC,
                ST_DISPLAY_PIN_SCK, ST_DISPLAY_PIN_MOSI);
        struct display_device *disp = light_display_st7789_create_device(
                "screentest_display_main", ST_DISPLAY_WIDTH, ST_DISPLAY_HEIGHT, io);
        _display[0] = disp;
        // both row_offset=20 and col_offset=20 were tried here and had zero visible effect
        // on a persistent noise strip -- but that test was confounded by screentest_common's
        // render context being hardcoded to the small OLED test rigs' 64x128 1bpp geometry
        // (now fixed, see ST_RENDER_WIDTH/HEIGHT/BPP in this app's screentest.h) and by a
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

        // backlight is a plain GPIO, not part of the SPI command set -- no existing
        // driver/ioport primitive covers it, so it's driven directly here
#if(LIGHT_SYSTEM == SYSTEM_PICO_SDK)
        gpio_init(ST_DISPLAY_PIN_BL);
        gpio_set_dir(ST_DISPLAY_PIN_BL, true);
        gpio_put(ST_DISPLAY_PIN_BL, true);
#endif

        light_info("display pipeline setup complete","");

        // shared I2C1 bus (also used by IMU/RTC, not yet implemented) -- setup_io_i2c
        // re-inits the same peripheral each time it's called, harmless as long as the
        // params (scl/sda) agree, which they will once IMU/RTC support lands
        struct io_context *touch_io = light_ioport_setup_io_i2c(
                PORT_I2C_1, ST_TOUCH_PIN_RST, CST816T_I2C_ADDR,
                ST_TOUCH_PIN_SCL, ST_TOUCH_PIN_SDA);
        _touch_main = light_touch_cst816t_create_device(
                "screentest_touch_main", ST_TOUCH_X_MAX, ST_TOUCH_Y_MAX, touch_io, ST_TOUCH_PIN_INT);

        light_info("touch pipeline setup complete","");
}
