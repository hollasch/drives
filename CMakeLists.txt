cmake_minimum_required(VERSION 3.2...3.27.7)

project (drives LANGUAGES CXX)

add_executable (drives drives.cpp)
target_link_libraries(drives Mpr.lib)

add_executable (display-volume-paths display-volume-paths.cpp)
target_link_libraries(display-volume-paths Mpr.lib)
