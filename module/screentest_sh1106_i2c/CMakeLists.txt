
add_executable(screentest_sh1106_i2c)

target_include_directories(
        screentest_sh1106_i2c PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        screentest_sh1106_i2c PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        screentest_sh1106_i2c PUBLIC
        light_display
        light_display_sh1106
        rend
        light_core
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        pico_add_extra_outputs(screentest_sh1106_i2c)
endif()
