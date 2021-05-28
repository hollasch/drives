cmake_minimum_required(VERSION 3.2)

project (drives LANGUAGES CXX)

add_executable (drives drives.cpp)
target_link_libraries(drives Mpr.lib)
