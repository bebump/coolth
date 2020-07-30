cmake_minimum_required(VERSION 3.12)

add_library(concurrentqueue INTERFACE)
target_include_directories(concurrentqueue INTERFACE ${CMAKE_CURRENT_LIST_DIR})
