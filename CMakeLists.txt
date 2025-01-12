# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.6)

# project name and languages
project(cmd_play VERSION 0.0 LANGUAGES CXX)

# UTF-8
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

##############################################################################
# OpenAL - Audio library
# Please set OPENALDIR environment variable.

find_package(OpenAL)

if(OpenAL_FOUND)
    # Tell the preprocessor OpenAL existence
    add_definitions(-DHAVE_OPENAL)

    # workaround for including <AL/alut.h>
    include_directories("${OPENAL_INCLUDE_DIR}" "${OPENAL_INCLUDE_DIR}/..")
endif()

##############################################################################
# alut

option(BUILD_EXAMPLES "build example applications" OFF)
option(BUILD_TESTS "build the test-suite" OFF)
if(OpenAL_FOUND)
    if(NOT BUILD_SHARED_LIBS)
        add_definitions(-DALUT_STATIC)
    endif()
    add_subdirectory(freealut)
endif()

##############################################################################
# pevent

add_subdirectory(pevent)

##############################################################################
# fmgon

add_subdirectory(fmgon)

##############################################################################
# Display current status

message(STATUS "OPENAL_INCLUDE_DIR: ${OPENAL_INCLUDE_DIR}")
message(STATUS "OPENAL_LIBRARY: ${OPENAL_LIBRARY}")
message(STATUS "BUILD_SHARED_LIBS: ${BUILD_SHARED_LIBS}")
message(STATUS "BUILD_EXAMPLES: ${BUILD_EXAMPLES}")
message(STATUS "BUILD_TESTS: ${BUILD_TESTS}")

##############################################################################
# cmd_play

# cmd_play.exe
add_executable(cmd_play cmd_play.cpp cmd_sing.cpp sound.cpp main.cpp soundplayer.cpp)
target_compile_definitions(cmd_play PRIVATE UNICODE _UNICODE JAPAN CMD_PLAY_EXE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(cmd_play fmgon alut shlwapi ${OPENAL_LIBRARY})

##############################################################################
