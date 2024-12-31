cmake_minimum_required(VERSION 3.13)
project(Render)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PyTorch
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

# Find Raylib
add_subdirectory(../../external/raylib raylib-build)

add_executable(render main.cpp ../shared/chinese_checkers.cpp)

target_include_directories(render PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ../../external/raylib/src/  # Include Raylib headers
    ${TORCH_INCLUDE_DIRS}
)

target_link_libraries(render ${TORCH_LIBRARIES} raylib)

if(APPLE)
    target_link_libraries(render
        "-framework OpenGL"
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreAudio"
    )
endif()