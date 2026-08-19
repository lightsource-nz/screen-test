
add_executable(screentest_ws_touch169)

target_include_directories(
        screentest_ws_touch169 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        screentest_ws_touch169 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        screentest_ws_touch169 PUBLIC
        screentest_common
        screentest_hw_ws_touch169
        light_draw
        light_core
)

# the background message-stream worker normally runs on core1 (see
# light_core_chip_rp2350/include/light_platform_port.h), but that races TinyUSB's device
# stack now that stdio is USB CDC (pico_stdio_usb drives tud_task() via its own background
# IRQ, independent of the app, and TinyUSB isn't safe to touch from two execution contexts
# at once) -- fall back to draining the message queue from the main loop instead, avoiding
# the cross-core TinyUSB access entirely
target_compile_definitions(
        screentest_ws_touch169 PUBLIC
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
        pico_add_extra_outputs(screentest_ws_touch169)
        # this board has no SWD pads exposed and UART console access has been unreliable
        # over an external debug probe -- USB CDC gives a serial console over the same
        # cable already used to flash the board, sidestepping probe/wiring entirely
        pico_enable_stdio_usb(screentest_ws_touch169 1)
        pico_enable_stdio_uart(screentest_ws_touch169 0)
endif()
