
add_executable(screentest_husb238)

target_sources(
        screentest_husb238 PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

#   light_ioport directly, unlike the display rigs here: those reach it transitively through
# light_display, and this app has no display at all. light_cli for the interactive console --
# the whole point of this rig is typing at it
target_link_libraries(
        screentest_husb238 PUBLIC
        light_ioport
        light_cli
        light_core
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        pico_add_extra_outputs(screentest_husb238)
endif()
