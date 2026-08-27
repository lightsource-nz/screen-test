
add_executable(screentest_ssd1322)

target_sources(
        screentest_ssd1322 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        screentest_ssd1322 PUBLIC
        light_display
        light_display_ssd1322
        light_draw
        light_core
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        pico_add_extra_outputs(screentest_ssd1322)
endif()
