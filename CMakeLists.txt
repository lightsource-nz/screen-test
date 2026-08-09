cmake_minimum_required(VERSION 3.18)

# front-load initialisers for light and pico-sdk
if(DEFINED ENV{LIGHT_PATH} AND (NOT LIGHT_PATH))
    set(LIGHT_PATH "$ENV{LIGHT_PATH}")
endif()
if(DEFINED ENV{PICO_SDK_PATH} AND (NOT PICO_SDK_PATH))
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}")
endif()
if(NOT LIGHT_PATH)
    set(LIGHT_PATH "${CMAKE_CURRENT_LIST_DIR}/../light_framework_mk3" CACHE PATH "path to the root directory of the light-framework project")
endif()
cmake_path(IS_RELATIVE LIGHT_PATH _LIGHT_PATH_RELATIVE)
if(${_LIGHT_PATH_RELATIVE})
        get_filename_component(LIGHT_PATH "${LIGHT_PATH}" REALPATH BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
endif()

# rend also defaults this, but relative to ITSELF -- and rend lives at module/rend here, so
# its guess lands on screen-test/module/font-crusher and misses a checkout sitting beside
# this project. Findcrush.cmake then quietly fetches font-crusher from GitHub instead, so a
# local edit to the crush tooling silently has no effect on this build. resolved here first,
# as a sibling of the project root, exactly as LIGHT_PATH is above
if(DEFINED ENV{FONT_CRUSHER_PATH} AND (NOT FONT_CRUSHER_PATH))
    set(FONT_CRUSHER_PATH "$ENV{FONT_CRUSHER_PATH}")
endif()
if(NOT FONT_CRUSHER_PATH)
    get_filename_component(_font_crusher_default "${CMAKE_CURRENT_LIST_DIR}/../font-crusher" REALPATH)
    if(EXISTS "${_font_crusher_default}/CMakeLists.txt")
        set(FONT_CRUSHER_PATH "${_font_crusher_default}" CACHE PATH "path to the root directory of the font-crusher project")
    endif()
endif()

include(${LIGHT_PATH}/external/light_preinit.cmake)

project(screen-test C CXX ASM)

add_subdirectory(${LIGHT_PATH}/ light_framework)

# modules
add_subdirectory(module/rend)
add_subdirectory(module/light_display)
add_subdirectory(module/light_ioport)
add_subdirectory(module/light_display_sh1106)
add_subdirectory(module/light_display_sh1107)
add_subdirectory(module/light_display_po13)
add_subdirectory(module/light_display_st7789)
add_subdirectory(module/light_touch)
add_subdirectory(module/light_touch_cst816t)

# light_button and light_ui are not submodules of this repo yet -- their remotes don't
# exist. resolve each from module/<name> if it's there (which is where `git submodule add`
# will put it, so converting them later needs no change here) and otherwise from a sibling
# checkout, the same way LIGHT_PATH and FONT_CRUSHER_PATH already resolve theirs
foreach(_ext_module light_button light_ui)
        string(TOUPPER ${_ext_module} _ext_var)
        if(NOT ${_ext_var}_PATH)
                if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/module/${_ext_module}/CMakeLists.txt")
                        set(${_ext_var}_PATH "${CMAKE_CURRENT_LIST_DIR}/module/${_ext_module}")
                else()
                        set(${_ext_var}_PATH "${CMAKE_CURRENT_LIST_DIR}/../${_ext_module}")
                endif()
        endif()
        get_filename_component(${_ext_var}_PATH "${${_ext_var}_PATH}" REALPATH)
        if(NOT EXISTS "${${_ext_var}_PATH}/CMakeLists.txt")
                message(FATAL_ERROR "could not find ${_ext_module} at ${${_ext_var}_PATH} -- set ${_ext_var}_PATH")
        endif()
        add_subdirectory(${${_ext_var}_PATH} ${_ext_module})
endforeach()

add_subdirectory(module/screentest_hw_po13)
add_subdirectory(module/screentest_hw_ws_touch169)
add_subdirectory(module/screentest_common)
add_subdirectory(module/light_ui_common)
add_subdirectory(module/screentest_po13)
add_subdirectory(module/light_ui_po13)
add_subdirectory(module/screentest_sh1106_spi4)
add_subdirectory(module/screentest_sh1106_i2c)
add_subdirectory(module/screentest_ws_touch169)
add_subdirectory(module/light_ui_touch169)
