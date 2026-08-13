#include <screentest_hw_mini_stm32h7.h>
#include <light_display_st7735.h>
#include <light_backlight.h>

#include <stm32h7xx.h>

// board: WeAct MiniSTM32H7xx (STM32H743VIT6) -- on-board 0.96in ST7735S panel, 160x80 in the
// landscape orientation it is mounted in, on SPI4. No touch, no IMU: this board's peripherals
// are a TF card slot, QSPI/SPI flash and a DVP camera port, none of which this demo uses.

//   THE BACKLIGHT IS NOT OPTIONAL, and this is the failure it causes: with it off the panel
// renders perfectly and shows nothing at all, which reads as a dead display rather than an
// unlit one. The first bring-up here did exactly that -- LED blinking, console reporting a
// healthy pipeline, screen black.
//   ACTIVE LOW, confirmed on hardware by driving the pin both ways over SWD. PE10 is TIM1_CH2N
// -- the COMPLEMENTARY output -- and WeAct start it with HAL_TIMEx_PWMN_Start() configured
// OCNPolarity = TIM_OCNPOLARITY_LOW. Reading their OCPolarity = HIGH and concluding active-high
// is the trap: that field describes CH2, which they do not use. Driving PE10 high turns the
// backlight OFF.
//   Driven through light_backlight's PWM driver now that light_platform has timer-backed PWM
// on this chip, so it dims rather than just switching. active_low is passed as true and
// light_backlight inverts the duty itself, which is why the platform layer below it stays a
// conventional PWM where a higher duty means more time high.
struct backlight_device *screentest_hw_mini_stm32h7_backlight(void)
{
        struct backlight_device *bl = light_backlight_pwm_create_device(
                        "screentest_backlight_main", ST_DISPLAY_PIN_BL, true);
        // full brightness to start, so the panel behaves exactly as it did when this was a
        // GPIO held low -- dimming is then something an application asks for rather than
        // something that changed underneath it
        light_backlight_set_level(bl, LIGHT_BACKLIGHT_LEVEL_MAX);
        return bl;
}

struct display_device *screentest_hw_mini_stm32h7_display(void)
{
        struct io_context *io = light_ioport_setup_io_spi_4p(
                ST_DISPLAY_SPI_PORT,
                ST_DISPLAY_PIN_RESET, ST_DISPLAY_PIN_CS, ST_DISPLAY_PIN_DC,
                ST_DISPLAY_PIN_SCK, ST_DISPLAY_PIN_MOSI);
        // before the device is created, so the initialisation sequence itself runs at the
        // working clock -- a panel that cannot take it then fails from the first frame rather
        // than only once something animates
        light_ioport_set_spi_clock(io, ST_DISPLAY_SPI_HZ);

        struct display_device *disp = light_display_st7735_create_device(
                "screentest_display_main", ST_DISPLAY_WIDTH, ST_DISPLAY_HEIGHT, io);

        //   the driver already defaults to these, but they are set explicitly here because
        // they are a property of THIS board's glass rather than of the controller -- a second
        // ST7735 board would want different ones, and the driver's default should not read as
        // a universal truth
        light_display_st7735_set_offset(disp, ST_DISPLAY_COL_OFFSET, ST_DISPLAY_ROW_OFFSET);
        //   creating the device cleared the panel already, but with whatever offset the driver
        // started with. Clearing again now the offset is settled avoids leaving a band of
        // powered-up GDDRAM outside the window that later updates never touch -- exactly the
        // stale-band problem the ws_touch169 board hit and spent a while diagnosing
        //   DIAGNOSTIC, temporary: fill with a solid known colour instead of black, and see
        // SCREENTEST_H7_DIAG_SOLID_FILL in the app, which then draws nothing over it. Red on
        // the glass proves the SPI transport, the ST7735 init sequence and the window/RAMWR
        // path all work, and moves the remaining fault into the canvas frame push. Still blank
        // proves the opposite. Black cannot distinguish those, which is why it is not black.
#if SCREENTEST_H7_DIAG_SOLID_FILL
        light_display_command_clear(disp, 0xF800);      // RGB565 red
#else
        light_display_command_clear(disp, 0);
#endif

        light_info("display pipeline setup complete","");
        return disp;
}
