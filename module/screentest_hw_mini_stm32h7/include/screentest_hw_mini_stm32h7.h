#ifndef _SCREENTEST_HW_MINI_STM32H7_H
#define _SCREENTEST_HW_MINI_STM32H7_H

#include <light_display.h>
#include <light_ioport.h>

//   WeAct MiniSTM32H7xx (STM32H743VIT6) with its on-board 0.96in TFT.
//
//   Every pin below comes from WeAct's own published board sources rather than from probing:
// their 03-LCD_Test example's CubeMX pin assignments and SPI configuration. Pins are written
// with LIGHT_IOPORT_PIN_STM32() because on STM32 a pin is a (port, pin) pair packed into the
// uint8_t light_ioport takes -- 0x4C is not recognisably PE12.

// SPI4, AF5. The panel is write-only: there is no MISO, and nothing reads back from it
#define ST_DISPLAY_SPI_PORT             4
#define ST_DISPLAY_PIN_SCK              LIGHT_IOPORT_PIN_STM32('E', 12)
#define ST_DISPLAY_PIN_MOSI             LIGHT_IOPORT_PIN_STM32('E', 14)
#define ST_DISPLAY_PIN_CS               LIGHT_IOPORT_PIN_STM32('E', 11)
#define ST_DISPLAY_PIN_DC               LIGHT_IOPORT_PIN_STM32('E', 13)
//   NO RESET GPIO. The panel's reset is not brought out -- WeAct's own driver defines its
// LCD_RST_SET/LCD_RST_RESET macros as empty. light_ioport_signal_reset() therefore skips the
// pulse rather than driving an unrelated pin, and the controller is reset by SWRESET instead
#define ST_DISPLAY_PIN_RESET            LIGHT_IOPORT_PIN_NONE

//   BACKLIGHT on PE10, ACTIVE HIGH -- and it is not optional. WeAct dim it with TIM1 channel 2
// in PWM1 mode at active-high polarity; this board module just holds the pin high, since
// light_platform's PWM is still stubbed on the STM32 ports and full brightness needs no PWM.
//   Leaving it undriven does not produce a dim display, it produces a BLACK one that is
// otherwise working perfectly -- which reads as a dead panel. That is exactly what the first
// bring-up here did.
#define ST_DISPLAY_PIN_BL               LIGHT_IOPORT_PIN_STM32('E', 10)

//   160x80 LANDSCAPE. The panel is natively 80x160 portrait and the driver's MADCTL puts it in
// landscape-rot180, which is how WeAct mount it -- so width and height are swapped relative to
// the controller's own idea of them
#define ST_DISPLAY_WIDTH                160
#define ST_DISPLAY_HEIGHT               80
#define ST_DISPLAY_BPP                  16

//   the GDDRAM offset for the landscape orientation, HannStar panel -- see the long note in
// light_display_st7735.h, which is where this actually gets explained. Confirmed on this
// board: a border at the outermost pixel ring sits flush against all four edges. If a future
// unit shows it displaced by two pixels with red/blue swapped, that one has the BOE panel and
// these become 0 and 24
#define ST_DISPLAY_COL_OFFSET           1
#define ST_DISPLAY_ROW_OFFSET           26

//   8MHz. The ST7735S datasheet allows a 66ns write cycle (~15MHz), and this stays a
// comfortable margin below it: the transport has no DMA yet, so the CPU is the bottleneck
// well before the bus is, and a display that is overclocked fails intermittently rather than
// cleanly
#define ST_DISPLAY_SPI_HZ               (8 * 1000 * 1000)

// user LED, active low -- confirmed on hardware, see light_board.h
#define ST_LED_PORT                     GPIOE
#define ST_LED_PIN                      3

//   TEMPORARY BRING-UP DIAGNOSTIC. 1 fills the panel with solid red at init and draws nothing
// afterwards, which separates two questions that a black screen cannot: does the SPI transport
// and ST7735 init sequence work at all (red appears), or does only the per-frame canvas push
// fail (red appears and never changes). Set back to 0 once the panel is known good.
#ifndef SCREENTEST_H7_DIAG_SOLID_FILL
#define SCREENTEST_H7_DIAG_SOLID_FILL   0
#endif

extern struct display_device *screentest_hw_mini_stm32h7_display(void);

#endif
