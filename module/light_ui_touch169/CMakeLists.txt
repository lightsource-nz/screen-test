
add_executable(light_ui_touch169)

target_include_directories(
        light_ui_touch169 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# 240x280 px over 27.9x32.6 mm -- derived from the panel's 1.69" diagonal at this pixel
# count, so crush resolves point size against the physical dimensions it actually has
crush_add_font_target(light_ui_touch169_font
        FONT ${FONT_CRUSHER_PATH}/test_resource/fonts/TypeLightSans.ttf
        DISPLAY_NAME ws_touch169 DISPLAY_WIDTH 240 DISPLAY_HEIGHT 280 DISPLAY_DIMENSION 27.9x32.6
        RENDER_NAME light_ui_touch169 POINT_SIZE 14 PIXEL_SIZE 16
)

target_sources(
        light_ui_touch169 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        light_ui_touch169 PUBLIC
        light_ui_common
        screentest_hw_ws_touch169
        light_ui_touch169_font
        light_ui
        light_touch
        light_imu
        light_audio
        light_backlight
        rend
        light_core
)

# see screentest_ws_touch169's CMakeLists for why the multicore worker is disabled on this
# board: the core1 message-stream worker races TinyUSB's device stack now that stdio is USB
# CDC, so the message queue is drained from the main loop instead
target_compile_definitions(
        light_ui_touch169 PUBLIC
        LIGHT_PLATFORM_HAS_MULTICORE_WORKER=0
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        pico_add_extra_outputs(light_ui_touch169)
        # this board has no SWD pads exposed and UART console access has been unreliable
        # over an external debug probe -- USB CDC gives a serial console over the same cable
        # already used to flash the board
        pico_enable_stdio_usb(light_ui_touch169 1)
        pico_enable_stdio_uart(light_ui_touch169 0)
endif()
