# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.6)

# project name and languages
project(pevent VERSION 1.0 LANGUAGES CXX)

# UTF-8
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

# libpevent.a
add_library(pevent STATIC
    _pevent_win32.cpp
    _pevent_posix.cpp)
target_include_directories(pevent PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

##############################################################################
