
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
        rend
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
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        pico_add_extra_outputs(screentest_ws_touch169)
        # this board has no SWD pads exposed and UART console access has been unreliable
        # over an external debug probe -- USB CDC gives a serial console over the same
        # cable already used to flash the board, sidestepping probe/wiring entirely
        pico_enable_stdio_usb(screentest_ws_touch169 1)
        pico_enable_stdio_uart(screentest_ws_touch169 0)
endif()
