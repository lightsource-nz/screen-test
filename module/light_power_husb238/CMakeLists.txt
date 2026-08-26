
add_library(light_power_husb238)

target_include_directories(
        light_power_husb238 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        light_power_husb238 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
        ${CMAKE_CURRENT_LIST_DIR}/src/power_husb238.c
)

target_link_libraries(
        light_power_husb238 PUBLIC
        light_power
        light_ioport
        light_core
)
