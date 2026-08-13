add_library(screentest_hw_mini_stm32h7 INTERFACE)

target_include_directories(
        screentest_hw_mini_stm32h7 INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        screentest_hw_mini_stm32h7 INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/hardware.c
)

target_link_libraries(
        screentest_hw_mini_stm32h7 INTERFACE
        light_display
        light_display_st7735
        light_ioport
        light_core
)
