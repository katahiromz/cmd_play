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
# pevent

add_subdirectory(pevent)

##############################################################################
# fmgon

add_subdirectory(fmgon)

##############################################################################
# cmd_play

# cmd_play.exe
add_executable(cmd_play cmd_play.cpp cmd_sing.cpp sound.cpp main.cpp soundplayer.cpp)
target_compile_definitions(cmd_play PRIVATE UNICODE _UNICODE JAPAN CMD_PLAY_EXE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(cmd_play fmgon shlwapi winmm)

##############################################################################
