cmake_minimum_required(VERSION 3.13)
project(chinese_checkers_ext)

# Find PyTorch
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# Add our shared library target
add_subdirectory(../shared)

# Set CUDA options if available
if(TORCH_CUDA_FOUND)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
endif()

# Add the extension library
add_library(chinese_checkers_ext MODULE
    bindings.cpp)

# Link against shared and PyTorch libraries
target_link_libraries(chinese_checkers_ext
    PUBLIC
        chinese_checkers_shared
        ${TORCH_LIBRARIES})

# Set include directories
target_include_directories(chinese_checkers_ext
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared
        ${TORCH_INCLUDE_DIRS})

# Set install path
install(TARGETS chinese_checkers_ext DESTINATION chinese_checkers_ext)