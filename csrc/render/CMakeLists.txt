cmake_minimum_required(VERSION 3.13)
project(Render)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Raylib
find_path(RAYLIB_INCLUDE_DIR raylib.h HINTS ../../external/raylib/src)
find_library(RAYLIB_LIBRARY raylib HINTS ../../external/raylib/src)

if (NOT RAYLIB_INCLUDE_DIR OR NOT RAYLIB_LIBRARY)
    message(FATAL_ERROR "Raylib not found. Run 'just setup' to clone Raylib.")
endif()

add_executable(render main.cpp)

target_include_directories(render PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ${RAYLIB_INCLUDE_DIR}
)

target_link_libraries(render ${RAYLIB_LIBRARY})