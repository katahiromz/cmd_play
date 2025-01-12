# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.6)

# Adopt new policy CMP0077
cmake_policy(SET CMP0077 NEW)

# enable testing
enable_testing()

# project name and languages
project(fmgon VERSION 0.0 LANGUAGES CXX)

# UTF-8
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

##############################################################################
# fmgon

# libfmgon.a
add_library(fmgon STATIC
    fmgen.cpp
    fmtimer.cpp
    opm.cpp
    opna.cpp
    psg.cpp
    YM2203.cpp
    YM2203_Timbre.cpp)
target_include_directories(fmgon PUBLIC ../freealut/include)
target_link_libraries(fmgon PRIVATE pevent ${OPENAL_LIBRARY} ${ALUT_LIBRARY})

##############################################################################
