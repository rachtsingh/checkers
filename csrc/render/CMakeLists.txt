cmake_minimum_required(VERSION 3.13)
project(chinese_checkers_render)

# Find Raylib (assuming it's in ../../external/raylib)
find_path(RAYLIB_INCLUDE_DIR raylib.h HINTS ../../external/raylib/src)
find_library(RAYLIB_LIBRARY raylib HINTS ../../external/raylib/src)

if(NOT RAYLIB_INCLUDE_DIR OR NOT RAYLIB_LIBRARY)
    message(FATAL_ERROR "Raylib not found. Run 'just setup' to clone Raylib.")
endif()

# Add our shared library target
add_subdirectory(../shared)

# Add the executable
add_executable(render main.cpp)

# Link against shared and Raylib libraries
target_link_libraries(render
    PUBLIC
        chinese_checkers_shared
        ${RAYLIB_LIBRARY})

# Set include directories
target_include_directories(render
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared
        ${RAYLIB_INCLUDE_DIR})

# Set install path (optional)
install(TARGETS render DESTINATION bin)