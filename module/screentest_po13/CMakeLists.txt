
add_executable(screentest_po13)

light_framework_application_module(screentest_po13)

target_include_directories(
        screentest_po13 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        screentest_po13 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        screentest_po13 PUBLIC
        light_display
        light_display_po13
        light_display_sh1107
        rend
        light_framework
        light_platform
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        pico_add_extra_outputs(screentest_po13)
endif()
