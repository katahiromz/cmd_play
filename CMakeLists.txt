# CMakeLists.txt --- CMake project settings
##############################################################################

# CMake minimum version
cmake_minimum_required(VERSION 3.6)

# project name, version, and languages
project(cmd_play VERSION 1.9 LANGUAGES CXX RC)

# UTF-8
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
endif()

##############################################################################
# CMake options

option(ENABLE_BEEP "Enable beep" ON)

##############################################################################
# pevent

add_subdirectory(pevent)

##############################################################################
# fmgon

add_subdirectory(fmgon)

##############################################################################
# cmd_play

# cmd_play.exe
add_executable(cmd_play cmd_play.cpp cmd_sing.cpp sound.cpp main.cpp soundplayer.cpp cmd_play_res.rc)
target_compile_definitions(cmd_play PRIVATE UNICODE _UNICODE JAPAN CMD_PLAY_EXE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(cmd_play fmgon shlwapi winmm)
if(ENABLE_BEEP)
    target_compile_definitions(cmd_play PRIVATE ENABLE_BEEP)
endif()

##############################################################################
