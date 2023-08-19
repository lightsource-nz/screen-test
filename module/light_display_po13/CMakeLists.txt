
add_library(light_display_po13)

target_include_directories(
        light_display_po13 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        light_display_po13 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
        ${CMAKE_CURRENT_LIST_DIR}/src/display_po13.c
)

target_link_libraries(
        light_display_po13 PUBLIC
        light_display
        rend
        light_framework
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        target_link_libraries(
                light_display_po13 PUBLIC
        )
endif()
