
add_executable(screentest_ws15rgb)

target_include_directories(
        screentest_ws15rgb PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# its screentest_config.h names this panel's geometry; everything else comes from
# screentest_common's defaults. the pinout comes from light_ui_hw_ws15rgb, which is also
# what the UI demo on this rig builds against
target_sources(
        screentest_ws15rgb PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/src/app.c
)

target_link_libraries(
        screentest_ws15rgb PUBLIC
        screentest_common
        light_ui_hw_ws15rgb
        light_draw
        light_core
)

if(LIGHT_SYSTEM STREQUAL PICO_SDK)
        pico_add_extra_outputs(screentest_ws15rgb)
endif()
