cmake_minimum_required(VERSION 3.20)
project(memLyrics CXX)

set(CMAKE_CXX_STANDARD 20)

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_executable(memLyrics main.cpp)
