
# INTERFACE, matching light_ui_common. as a static library this compiled app.c once on its
# own behalf as well as into every consumer, and that library copy was dead weight: each
# app's own compilation wins at link time (objects handed to the linker directly are
# preferred over archive members), which is exactly what lets an app configure the shared
# demo for its own board. the archive copy could never be the one that ran, and now that the
# demo needs a per-app screentest_config.h it could not even compile
add_library(screentest_common INTERFACE)

target_include_directories(
        screentest_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        screentest_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        screentest_common INTERFACE
        light_canvas
        light_imu
        light_backlight
        light_display
        light_touch
        rend
        light_core
)
