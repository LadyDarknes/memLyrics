cmake_minimum_required(VERSION 3.20)
project(memLyrics CXX)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

if(MSVC)
    add_compile_options(/utf-8 /EHsc)
endif()

set(SOURCES
    src/main.cpp
    src/player.cpp
    src/lyrics.cpp
)

add_executable(memLyrics ${SOURCES})
target_include_directories(memLyrics PRIVATE include)
target_link_libraries(memLyrics PRIVATE nlohmann_json::nlohmann_json wininet windowsapp)
