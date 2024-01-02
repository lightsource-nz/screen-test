
add_library(screentest_common)

light_framework_application_module(screentest_common)

target_include_directories(
        screentest_common PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        screentest_common PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        screentest_common PUBLIC
        light_display
        rend
        light_framework
        light_platform
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        pico_add_extra_outputs(screentest_common)
endif()
