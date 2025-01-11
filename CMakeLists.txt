# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.6)

# project name and languages
project(cmd_sing VERSION 0.5.1 LANGUAGES CXX)

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
# cmd_sing

# cmd_sing.exe
add_executable(cmd_sing cmd_sing.cpp sound.cpp main.cpp)
target_compile_definitions(cmd_sing PRIVATE UNICODE _UNICODE JAPAN CMD_SING_EXE)
target_link_libraries(cmd_sing fmgon alut shlwapi ${OPENAL_LIBRARY})

##############################################################################
