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

# defines BUILD_TESTING (default ON) and calls enable_testing(). the modules that carry tests
# gate them on BUILD_TESTING *and* a host build, so this costs nothing on the target trees --
# a cross-compiled test binary could not be run to say anything anyway.
#
# so: `ctest` in build-host runs them; in build or build-waveshare-touch169 there are simply
# none registered
include(CTest)

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

# all of these are submodules under module/ now, which is what this resolver finds first. it
# was written when they were sibling checkouts with no remotes, and the sibling fallback is
# kept deliberately: it is what lets one of them be developed against a working copy outside
# the submodule without editing the build. set <NAME>_PATH to point somewhere else entirely
foreach(_ext_module light_audio light_backlight light_button light_canvas light_imu light_imu_qmi8658 light_ui)
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

add_subdirectory(module/screentest_common)
add_subdirectory(module/light_ui_demo_common)

# the WeAct MiniSTM32H7xx and its on-board ST7735S panel. In-tree rather than submodules for
# now -- published once proven on hardware, like every other driver here
if(LIGHT_SYSTEM STREQUAL CMSIS AND LIGHT_BOARD STREQUAL mini_stm32h7)
        add_subdirectory(module/light_display_st7735)
        add_subdirectory(module/screentest_hw_mini_stm32h7)
        add_subdirectory(module/screentest_mini_stm32h7)
endif()

#   the RP2 boards' hardware modules and the applications built on them. Guarded because every
# one of them is for a specific Pico-family board: on any other system they pull in pico-sdk
# headers, and the PWM-backed device libraries they link have no backing peripheral. Building
# them for an STM32 board is not a thing that could work, and the failure -- six undefined
# light_platform_pwm_* references at link, naming nothing that leads back to the board -- says
# so very badly.
if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        add_subdirectory(module/screentest_hw_po13)
        add_subdirectory(module/screentest_hw_ws_touch169)
        add_subdirectory(module/screentest_po13)
        add_subdirectory(module/light_ui_demo_po13)
        add_subdirectory(module/screentest_sh1106_spi4)
        add_subdirectory(module/screentest_sh1106_i2c)
        add_subdirectory(module/screentest_ws_touch169)
        add_subdirectory(module/light_ui_demo_touch169)
        # measures the touch panel's visible corner radius, which the glass hides part of -- see
        # its app.c. a bring-up tool rather than a demo, kept because the constant it produces
        # (ST_DISPLAY_CORNER_RADIUS) is otherwise a guess, and a wrong guess is invisible in code
        add_subdirectory(module/screentest_calib169)
endif()
