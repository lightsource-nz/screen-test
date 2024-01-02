#include <screentest.h>
#include <light_display_po13.h>

void __screentest_hardware_init();

// app: screentest_po13
// defines two pico-oled-1.3 displays, one attached to the default pins
// for the display board, and one using spi port 0 and pins 16-20

void __screentest_hardware_init()
{
        struct io_context *io_main =
                light_display_po13_setup_io_spi_4p(PORT_SPI_1);
        struct display_device *disp_main =
                light_display_po13_create_device(
                        "screentest_display_main", io_main);
        display[0] = disp_main;
        struct io_context *io_sec =
                light_display_ioport_setup_io_spi_4p(
                        PORT_SPI_0,
                        ST_DISPLAY_1_PIN_RESET,
                        ST_DISPLAY_1_PIN_CS,
                        ST_DISPLAY_1_PIN_DC,
                        ST_DISPLAY_1_PIN_SCK,
                        ST_DISPLAY_1_PIN_TX);
        struct display_device *display_sec =
                light_display_sh1107_create_device(
                        "screentest_display_sec", 64, 128, 1, io_sec);
        display[1] = display_sec;
        light_info("display pipeline setup complete","");
}