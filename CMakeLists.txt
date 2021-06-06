cmake_minimum_required(VERSION 3.15)

project(bebump_coolth VERSION 0.9.7)

add_subdirectory(bbmp_windows)
add_subdirectory(extern/JUCE)
add_subdirectory(extern/cereal)

juce_add_gui_app(bebump_coolth PRODUCT_NAME "Bebump Coolth" ICON_BIG
                 resources/systray.png)

target_sources(
  bebump_coolth
  PRIVATE src/components/graph_editor.cpp
          src/components/log_component.cpp
          src/components/log_component.h
          src/components/multi_graph_editor.cpp
          src/components/multi_graph_editor.h
          src/juce_priorizable_thread.h
          src/main.cpp
          src/main_component.cpp
          src/settings.h
          src/components/custom_slider.cpp
          src/components/custom_slider.h
          src/components/graph_editor.h)

target_compile_features(bebump_coolth PUBLIC cxx_std_17)

target_compile_definitions(
  bebump_coolth
  PRIVATE
    _ENABLE_ATOMIC_ALIGNMENT_FIX=1
    # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might
    # not need them.
    JUCE_WEB_BROWSER=0 # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the
    # `juce_add_gui_app` call
    JUCE_USE_CURL=0 # If you remove this, add `NEEDS_CURL TRUE` to the
    # `juce_add_gui_app` call
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:bebump_coolth,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:bebump_coolth,JUCE_VERSION>"
)

target_link_libraries(bebump_coolth PRIVATE bbmp::bbmp_windows)
target_link_libraries(bebump_coolth PRIVATE juce::juce_gui_extra)
target_link_libraries(bebump_coolth PRIVATE cereal::cereal)

juce_add_binary_data(bebump_coolth_data SOURCES resources/systray.png
                     resources/button_info.png)
target_link_libraries(bebump_coolth PRIVATE bebump_coolth_data)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/../cmake CACHE)

include(FindPathBetter)

if(${CMAKE_SIZEOF_VOID_P} STREQUAL 8)
  set(MT_EXE_FILTER x64)
endif()

find_path_better(
  MT_EXE_PATH mt.exe PATHS "C:/Program Files (x86)/Windows Kits/10/bin" FILTER
  ${MT_EXE_FILTER})

if(MT_EXE_PATH)
  set(MT_EXE ${MT_EXE_PATH}/mt.exe)
else()
  message(FATAL_ERROR "Couldn't find mt.exe. Is Windows SDK installed?")
endif()

add_custom_command(
  TARGET bebump_coolth
  POST_BUILD
  COMMAND ${MT_EXE} -manifest ${CMAKE_CURRENT_SOURCE_DIR}/app.manifest
          -outputresource:$<TARGET_FILE:bebump_coolth>)

install(
  TARGETS bebump_coolth
  CONFIGURATIONS Release
  RUNTIME DESTINATION .)

# >>> =========================================================================

# juce_add_gui_app(graph_editor PRODUCT_NAME "Graph Editor")
#
# target_sources( graph_editor PRIVATE src/components/graph_editor.cpp
# src/components/graph_editor.h src/components/labeled_multi_graph_editor.h
# src/components/labeled_multi_graph_editor.cpp src/components/log_component.cpp
# src/components/log_component.h src/graph_main_component.cpp
# src/graph_main_component.h src/graph_main.cpp)
#
# target_compile_features(graph_editor PUBLIC cxx_std_17)
#
# target_compile_definitions( graph_editor PRIVATE
# _ENABLE_ATOMIC_ALIGNMENT_FIX=1 # JUCE_WEB_BROWSER and JUCE_USE_CURL would be
# on by default, but you might # not need them. JUCE_WEB_BROWSER=0 # If you
# remove this, add `NEEDS_WEB_BROWSER TRUE` to the # `juce_add_gui_app` call
# JUCE_USE_CURL=0 # If you remove this, add `NEEDS_CURL TRUE` to the #
# `juce_add_gui_app` call
# JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:graph_editor,JUCE_PRODUCT_NAME>"
# JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:graph_editor,JUCE_VERSION>"
# )
#
# target_link_libraries(graph_editor PRIVATE juce::juce_gui_extra)
# target_link_libraries(graph_editor PRIVATE bbmp_windows)
# target_link_libraries(graph_editor PRIVATE cereal::cereal)
