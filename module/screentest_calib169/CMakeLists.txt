
add_executable(screentest_calib169)

target_include_directories(
        screentest_calib169 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# same font and metrics as the UI demo on this board -- the calibration only needs to print a
# two-digit number, but matching the demo means the glyph box it is read in is the same one
# the demo's title will have to fit inside the curve
crush_add_font_target(screentest_calib169_font
        FONT ${FONT_CRUSHER_PATH}/test_resource/fonts/TypeLightSans.ttf
        DISPLAY_NAME ws_touch169 DISPLAY_WIDTH 240 DISPLAY_HEIGHT 280 DISPLAY_DIMENSION 27.9x32.6
        RENDER_NAME screentest_calib169 POINT_SIZE 14 PIXEL_SIZE 16
)

target_sources(
        screentest_calib169 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        screentest_calib169 PUBLIC
        screentest_hw_ws_touch169
        screentest_calib169_font
        light_canvas
        light_display
        light_backlight
        rend
        light_core
)

# see screentest_ws_touch169's CMakeLists: the core1 message-stream worker races TinyUSB's
# device stack now that stdio is USB CDC, so the queue is drained from the main loop instead
target_compile_definitions(
        screentest_calib169 PUBLIC
        LIGHT_PLATFORM_HAS_MULTICORE_WORKER=0
        #   the CDC transmit FIFO, 64 bytes by default at full speed. stdio_usb_out_chars()
        # gives up and DISCARDS the remainder of a write when the FIFO cannot drain, silently
        # and mid-line, so any console line longer than about 128 characters lost its tail and
        # ran into the next one. Measured with a 166-character ruler line on the touch board:
        # cut at exactly 128 with the default, complete at 512.
        #
        #   how much is lost depends entirely on how many long lines an app emits -- for this
        # boot log it was one line and four characters, not a flood. The reason to fix it is
        # that the loss is silent and lands mid-line, not that it is voluminous.
        #
        #   512 rather than 256 to sit clear of the framework log buffer (256), for under half
        # a kilobyte of RAM
        CFG_TUD_CDC_TX_BUFSIZE=512
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        pico_add_extra_outputs(screentest_calib169)
        pico_enable_stdio_usb(screentest_calib169 1)
        pico_enable_stdio_uart(screentest_calib169 0)
endif()
