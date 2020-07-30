cmake_minimum_required(VERSION 3.15)

project(bbmp_windows)

set(src ${CMAKE_CURRENT_LIST_DIR}/src/bbmp_windows/bbmp)
add_library(
  bbmp_windows STATIC
  ${src}/child_process.cpp
  ${src}/child_process.h
  ${src}/fan_controller_communicator.h
  ${src}/line_reader.cpp
  ${src}/line_reader.h
  ${src}/logging.cpp
  ${src}/logging.h
  ${src}/recreate_on_failure.h
  ${src}/serial.cpp
  ${src}/serial.h
  ${src}/stringstream.cpp
  ${src}/stringstream.h
  ${src}/windows_handles.cpp
  ${src}/windows_handles.h)

target_compile_features(bbmp_windows PUBLIC cxx_std_17)
target_include_directories(bbmp_windows
                           INTERFACE ${CMAKE_CURRENT_LIST_DIR}/src/bbmp_windows)
target_compile_options(bbmp_windows PUBLIC /EHsc)

add_subdirectory(extern/moodycamel) # For logging
target_link_libraries(bbmp_windows PRIVATE concurrentqueue)

add_library(bbmp::bbmp_windows ALIAS bbmp_windows)

# <<< BBMP_WINDOWS ------------------------------------------------------------

add_executable(test src/main.cpp)
target_link_libraries(test bbmp_windows)