
add_library(io_command)

target_include_directories(
        io_command PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        io_command PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
)

target_link_libraries(
        io_command PUBLIC
        light_framework
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        target_link_libraries(
                io_command PUBLIC
                hardware_gpio
                hardware_spi
                hardware i2c
        )
endif()
