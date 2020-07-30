cmake_minimum_required(VERSION 3.6...3.15)

project(cereal LANGUAGES CXX VERSION 1.3.0)

add_library(cereal INTERFACE)
add_library(cereal::cereal ALIAS cereal)
target_include_directories(cereal INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
