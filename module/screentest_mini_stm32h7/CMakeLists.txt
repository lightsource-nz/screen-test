add_executable(screentest_mini_stm32h7)

target_sources(
        screentest_mini_stm32h7 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        screentest_mini_stm32h7 PUBLIC
        screentest_hw_mini_stm32h7
        light_display_st7735
        light_backlight
        light_display
        light_ioport
        light_canvas
        rend
        light_core
)

# .bin/.hex plus the size breakdown, through the property hook cmake/cmsis.cmake installs
light_target_set_property(screentest_mini_stm32h7 application_target 1)
