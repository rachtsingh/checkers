cmake_minimum_required(VERSION 3.13)
project(Generate)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find PyTorch
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

add_executable(generate main.cpp ../shared/chinese_checkers.cpp)

target_include_directories(generate PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared
    ${TORCH_INCLUDE_DIRS}
)

target_link_libraries(generate ${TORCH_LIBRARIES})