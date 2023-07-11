cmake_minimum_required(VERSION 3.12)

include(${LIGHT_PATH}/external/light_preinit.cmake)

project(screen-test C CXX ASM)

add_subdirectory(${LIGHT_PATH}/ light_framework)

# modules
add_subdirectory(module/rend)
add_subdirectory(module/screentest_main)
