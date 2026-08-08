
add_library(screentest_ui_common INTERFACE)

target_include_directories(
        screentest_ui_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(
        screentest_ui_common INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

# no light_touch/light_button here: which input modules exist is a property of the board,
# and each app names its own in its Light_Application_Define
target_link_libraries(
        screentest_ui_common INTERFACE
        light_ui
        light_display
        rend
        light_core
)
