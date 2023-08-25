
add_library(light_display_po13)

if(LIGHT_PLATFORM STREQUAL HOST)
        set(LIGHT_PORT host)
elseif(LIGHT_SYSTEM STREQUAL PICO_SDK AND LIGHT_PLATFORM STREQUAL TARGET)
        set(LIGHT_PORT rp2)
endif()

target_include_directories(
        light_display_po13 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        light_display_po13 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
        ${CMAKE_CURRENT_LIST_DIR}/src/display_po13.c
        ${CMAKE_CURRENT_LIST_DIR}/src/portable/${LIGHT_PORT}/platform.c
)

target_link_libraries(
        light_display_po13 PUBLIC
        light_display
        rend
        light_framework
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK AND LIGHT_PLATFORM STREQUAL TARGET)
        target_link_libraries(
                light_display_po13 PUBLIC
                pico_stdlib
                hardware_i2c
                hardware_spi
        )
endif()
