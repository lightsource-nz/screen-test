
add_library(light_power)

target_include_directories(
        light_power PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        light_power PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
        ${CMAKE_CURRENT_LIST_DIR}/src/power.c
)

# light_ioport for the transport, exactly as light_touch and light_imu depend on it: struct
# io_context is fully generic and carries no display or pixel coupling
target_link_libraries(
        light_power PUBLIC
        light_ioport
        light_core
)
