cmake_minimum_required(VERSION 3.18)

# front-load initialisers for light and pico-sdk
if(DEFINED ENV{LIGHT_PATH} AND (NOT LIGHT_PATH))
    set(LIGHT_PATH "$ENV{LIGHT_PATH}")
endif()
if(DEFINED ENV{PICO_SDK_PATH} AND (NOT PICO_SDK_PATH))
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if(LIGHT_PATH)
        cmake_path(IS_RELATIVE LIGHT_PATH _LIGHT_PATH_RELATIVE)
        if(${_LIGHT_PATH_RELATIVE})
                get_filename_component(LIGHT_PATH "${LIGHT_PATH}" REALPATH BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
        endif()
endif()

include(${LIGHT_PATH}/external/light_preinit.cmake)

project(screen-test C CXX ASM)

add_subdirectory(${LIGHT_PATH}/ light_framework)

# modules
add_subdirectory(module/rend)
add_subdirectory(module/light_display)
add_subdirectory(module/light_display_ioport)
add_subdirectory(module/light_display_sh1106)
add_subdirectory(module/light_display_sh1107)
add_subdirectory(module/light_display_po13)
add_subdirectory(module/screentest_common)
add_subdirectory(module/screentest_po13)
add_subdirectory(module/screentest_sh1106_spi4)
