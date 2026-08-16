cmake_minimum_required(VERSION 3.18)

# front-load initialisers for light and pico-sdk
#   CACHE, not a normal variable, and the distinction is load-bearing. module/rend/CMakeLists.txt
# also declares LIGHT_PATH with `set(... CACHE PATH ...)`, defaulting it relative to ITSELF --
# which lands on screen-test/module/light_framework_mk3, a path that does not exist. Creating a
# cache entry REMOVES any normal variable of the same name so the cached value is visible, so a
# plain `set(LIGHT_PATH ...)` here is silently discarded the moment rend is added, and the build
# fails looking for light_preinit.cmake under module/.
#   the fallback below always cached its value, so this only bites when LIGHT_PATH is set in the
# ENVIRONMENT -- which is exactly what the script layer does. Every existing build tree here was
# configured before that was true, which is why they work and a fresh one did not.
if(DEFINED ENV{LIGHT_PATH} AND (NOT LIGHT_PATH))
    set(LIGHT_PATH "$ENV{LIGHT_PATH}" CACHE PATH "path to the root directory of the light-framework project")
endif()
if(DEFINED ENV{PICO_SDK_PATH} AND (NOT PICO_SDK_PATH))
    set(PICO_SDK_PATH "$ENV{PICO_SDK_PATH}" CACHE PATH "path to the pico-sdk checkout")
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
#   CACHE for the same reason as LIGHT_PATH above: rend declares FONT_CRUSHER_PATH as a cache
# entry too, so an uncached value here is removed as soon as rend is added and its own guess
# wins -- at which point Findcrush.cmake fetches font-crusher from GitHub and local edits to the
# crush tooling silently stop affecting this build
if(DEFINED ENV{FONT_CRUSHER_PATH} AND (NOT FONT_CRUSHER_PATH))
    set(FONT_CRUSHER_PATH "$ENV{FONT_CRUSHER_PATH}" CACHE PATH "path to the root directory of the font-crusher project")
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

#   the display group -- core, six panel drivers and backlight. These were seven separate
# repositories and seven submodule pointers here, kept in step by hand; they are now one
# project, resolved by path rather than vendored. Nothing needs configuring for the normal
# layout: light_resolve_project() falls back to a sibling checkout at ../light_display.
#
#   adding it defines every module in the group, including drivers this project never links
# (there is no ST7735 panel on any RP2 board here). That is free -- they are INTERFACE
# libraries and compile nothing until something links them -- and it is the point: which
# drivers exist is the group's business, and no longer a list this file has to maintain.
#
#   light_ioport is deliberately absent from this list. It moved into the framework, so it
# arrives with add_subdirectory(${LIGHT_PATH}) above.
#
#   MARKER, because the group and one of its own modules share the name light_display. The
# resolver looks for an in-project module/<name> before the sibling, and module/light_display is
# exactly what a checkout of the MODULE would be called -- this repository had one until the
# consolidation. Without a marker that only the group has, restoring that directory for any
# reason would silently shadow the group, and the build would fail hunting for the six drivers
# that had never been added.
light_resolve_project(LIGHT_DISPLAY light_display MARKER module/light_display/CMakeLists.txt)
add_subdirectory(${LIGHT_DISPLAY_PATH} light_display_group)

add_subdirectory(module/light_touch)
add_subdirectory(module/light_touch_cst816t)

#   still one repository each, and still submodules under module/, which is what
# light_resolve_project() finds first. The sibling fallback is what lets any one of them be
# developed against a working copy outside the submodule without editing this file; set
# <NAME>_PATH to point somewhere else entirely.
foreach(_ext_module light_audio light_button light_canvas light_imu light_imu_qmi8658 light_ui)
        string(TOUPPER ${_ext_module} _ext_var)
        light_resolve_project(${_ext_var} ${_ext_module})
        add_subdirectory(${${_ext_var}_PATH} ${_ext_module})
endforeach()

add_subdirectory(module/screentest_common)
add_subdirectory(module/light_ui_demo_common)

# the WeAct MiniSTM32H7xx and its on-board ST7735S panel. The ST7735 driver itself is no longer
# named here -- it comes with the display group, like every other panel driver
if(LIGHT_SYSTEM STREQUAL CMSIS AND LIGHT_BOARD STREQUAL mini_stm32h7)
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
        #   these stay unguarded past PICO_SDK ON PURPOSE, and the distinction from the touch169
        # block below is the whole point. The po13 modules target the Waveshare Pico-OLED 1.3"
        # EXPANSION board: the invariant is the expansion board, not the host, so they build and
        # run on any Pico-compatible target -- rp2040 Pico and Pico 2 variants alike. Narrowing
        # them to a PICO_BOARD the way touch169 is narrowed would be wrong, and wrong in a way
        # that looks tidy: it would drop working builds for every host but one.
        #
        #   the test to apply is which piece of hardware is fixed. An expansion board fixes the
        # peripheral and leaves the host free; an integrated board fixes both
        add_subdirectory(module/screentest_hw_po13)
        add_subdirectory(module/screentest_po13)
        add_subdirectory(module/light_ui_demo_po13)
        add_subdirectory(module/screentest_sh1106_spi4)
        add_subdirectory(module/screentest_sh1106_i2c)

        #   the Waveshare RP2350-Touch-LCD-1.69 modules. Everything specific to that hardware is
        # RP2350-only and always will be: there is one revision of the board, so there is exactly
        # one PICO_BOARD value that builds any of this, and matching on it is both the narrowest
        # and the most durable test available. Guarding on the CHIP instead would be wrong in the
        # direction that matters -- it would keep letting these build for any other RP2350 board,
        # which is where the panel, the touch controller and the pin map all stop being true.
        #
        #   what went wrong without this: light_ui_demo_touch169 was built for an rp2040 tree and
        # failed at link with `undefined reference to light_core_port_panic'. That is the SDK
        # panic hook it installs, and it exists solely in light_core_chip_rp2350 along with the
        # core 1 USB worker it hands printing to -- the rp2040 port does not mention
        # LIGHT_PLATFORM_USB_ON_CORE1 at all. So the app could never have worked there; the panic
        # symbol was just the first thing the linker happened to notice, and it names nothing
        # that leads back to the board. Exactly the badly-worded failure the comment above is
        # about, one level further down.
        #
        #   the other three are guarded on the same condition even though they link cleanly for
        # rp2040 today. Linking is not the bar -- they drive a panel, a touch controller and a
        # pin map that only exist on this board, so a build of them for anything else produces a
        # binary that cannot run. Leaving them out on the grounds that they happen to link would
        # be preserving builds whose only property is that nobody has tried them yet
        if(PICO_BOARD STREQUAL waveshare_rp2350_touch_lcd_1.69)
                add_subdirectory(module/screentest_hw_ws_touch169)
                add_subdirectory(module/screentest_ws_touch169)
                add_subdirectory(module/light_ui_demo_touch169)
                # measures the touch panel's visible corner radius, which the glass hides part of
                # -- see its app.c. a bring-up tool rather than a demo, kept because the constant
                # it produces (ST_DISPLAY_CORNER_RADIUS) is otherwise a guess, and a wrong guess
                # is invisible in code
                add_subdirectory(module/screentest_calib169)
        endif()
endif()
