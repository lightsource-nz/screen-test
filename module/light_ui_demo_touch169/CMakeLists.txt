
add_executable(light_ui_demo_touch169)

target_include_directories(
        light_ui_demo_touch169 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# 240x280 px over 27.9x32.6 mm -- derived from the panel's 1.69" diagonal at this pixel
# count, so crush resolves point size against the physical dimensions it actually has
crush_add_font_target(light_ui_demo_touch169_font
        FONT ${FONT_CRUSHER_PATH}/test_resource/fonts/TypeLightSans.ttf
        DISPLAY_NAME ws_touch169 DISPLAY_WIDTH 240 DISPLAY_HEIGHT 280 DISPLAY_DIMENSION 27.9x32.6
        RENDER_NAME light_ui_demo_touch169 POINT_SIZE 14 PIXEL_SIZE 16
)

target_sources(
        light_ui_demo_touch169 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        light_ui_demo_touch169 PUBLIC
        light_ui_demo_common
        screentest_hw_ws_touch169
        light_ui_demo_touch169_font
        light_ui
        light_touch
        light_imu
        light_audio
        light_backlight
        rend
        # the board has no console, so this is here for the build-time command baked in by
        # LIGHT_BOOT_COMMAND rather than for anything typed at runtime
        light_cli
        light_core
)

# see screentest_ws_touch169's CMakeLists for why the multicore worker is disabled on this
# board: the core1 message-stream worker races TinyUSB's device stack now that stdio is USB
# CDC, so the message queue is drained from the main loop instead
target_compile_definitions(
        light_ui_demo_touch169 PUBLIC
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
        pico_add_extra_outputs(light_ui_demo_touch169)
        # this board has no SWD pads exposed and UART console access has been unreliable
        # over an external debug probe -- USB CDC gives a serial console over the same cable
        # already used to flash the board
        pico_enable_stdio_usb(light_ui_demo_touch169 1)
        pico_enable_stdio_uart(light_ui_demo_touch169 0)
endif()
