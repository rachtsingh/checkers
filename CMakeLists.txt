cmake_minimum_required(VERSION 3.10)
project(ChineseCheckersBinaries)

# Find Torch
find_package(Torch REQUIRED)

# Include shared headers for both binaries
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/csrc/shared)

# Render executable
add_executable(render_exe csrc/render/main.cpp)
target_link_libraries(render_exe "${TORCH_LIBRARIES}")
set_property(TARGET render_exe PROPERTY CXX_STANDARD 14)

# Generate executable
add_executable(generate_exe csrc/generate/main.cpp)
target_link_libraries(generate_exe "${TORCH_LIBRARIES}")
set_property(TARGET generate_exe PROPERTY CXX_STANDARD 14)
