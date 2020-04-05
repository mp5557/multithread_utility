cmake_minimum_required(VERSION 3.1)
project(multithread_utility)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions("-DFMT_HEADER_ONLY" "-std=c++11")

add_subdirectory(3rd_libraries)
add_subdirectory(libraries)
add_subdirectory(main)
