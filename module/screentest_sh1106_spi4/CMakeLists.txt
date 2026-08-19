
add_executable(screentest_sh1106_spi4)

target_include_directories(
        screentest_sh1106_spi4 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        screentest_sh1106_spi4 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        screentest_sh1106_spi4 PUBLIC
        light_display
        light_display_sh1106
        light_draw
        light_core
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        pico_add_extra_outputs(screentest_sh1106_spi4)
endif()
