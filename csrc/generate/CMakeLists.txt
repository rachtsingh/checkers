cmake_minimum_required(VERSION 3.13)
project(chinese_checkers_generate)

# Find PyTorch
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# Add our shared library target
add_subdirectory(../shared)

# Add the executable
add_executable(generate main.cpp)

# Link against shared and PyTorch libraries
target_link_libraries(generate
    PUBLIC
        chinese_checkers_shared
        ${TORCH_LIBRARIES})

# Set include directories
target_include_directories(generate
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared
        ${TORCH_INCLUDE_DIRS})

# Set install path (optional)
install(TARGETS generate DESTINATION bin)