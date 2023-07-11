
add_executable(screentest_main)

light_framework_application_module(screentest_main)

target_include_directories(
        screentest_main PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        screentest_main PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        screentest_main PUBLIC
        light_framework
        rend
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        pico_add_extra_outputs(screentest_main)
endif()
