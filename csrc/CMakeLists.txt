cmake_minimum_required(VERSION 3.13)
project(ChineseCheckers)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PyTorch
find_package(Torch REQUIRED)

# Build the shared library
add_subdirectory(shared)

# Build the PyTorch extension (needs to be configured via setup.py)
add_subdirectory(ext)

# Find Raylib (assuming it's in ../../external/raylib)
find_path(RAYLIB_INCLUDE_DIR raylib.h HINTS ../../external/raylib/src)
find_library(RAYLIB_LIBRARY raylib HINTS ../../external/raylib/src)

if(NOT RAYLIB_INCLUDE_DIR OR NOT RAYLIB_LIBRARY)
    message(FATAL_ERROR "Raylib not found. Run 'just setup' to clone Raylib.")
endif()

# Build generate binary
add_subdirectory(generate)

# Build render binary
add_subdirectory(render)