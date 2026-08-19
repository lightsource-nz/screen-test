
add_executable(screentest_po13)

target_include_directories(
        screentest_po13 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# no screentest.h of its own: its screentest_config.h is empty, so it picks up
# screentest_common's defaults (64x128 1bpp, LIGHT_DRAW_ROTATE_90), which are this board's real
# geometry. its pinout comes from screentest_hw_po13 instead
target_sources(
        screentest_po13 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        screentest_po13 PUBLIC
        screentest_common
        screentest_hw_po13
        light_draw
        light_core
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        pico_add_extra_outputs(screentest_po13)
endif()
