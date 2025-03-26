cmake_minimum_required(VERSION 3.10)
project(ChineseCheckersBinaries)

# Set Ninja as the preferred generator
if(NOT DEFINED CMAKE_GENERATOR)
  set(CMAKE_GENERATOR "Ninja")
endif()

# Optionally set Torch's CMake prefix if not provided externally.
if(NOT DEFINED CMAKE_PREFIX_PATH)
  # Adjust this path to your virtual environment's Torch share directory.
  set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/.venv/lib/python3.11/site-packages/torch/share/cmake/Torch")
endif()

find_package(Torch REQUIRED)

# Add raylib as a subdirectory so its target is built
add_subdirectory(${CMAKE_SOURCE_DIR}/external/raylib raylib-build)

include_directories(${CMAKE_SOURCE_DIR}/csrc/shared)

# Render executable (links against raylib)
add_executable(render
  ${CMAKE_SOURCE_DIR}/csrc/render/main.cpp
  ${CMAKE_SOURCE_DIR}/csrc/shared/chinese_checkers.cpp
)
target_include_directories(render PUBLIC
  ${CMAKE_SOURCE_DIR}/csrc/render
  ${CMAKE_SOURCE_DIR}/csrc/shared
  ${CMAKE_SOURCE_DIR}/external/raylib/src  # Raylib headers
  ${TORCH_INCLUDE_DIRS}
)
target_link_libraries(render "${TORCH_LIBRARIES}" raylib)
set_property(TARGET render PROPERTY CXX_STANDARD 20)

# Generate executable (doesn't require raylib)
add_executable(generate
  ${CMAKE_SOURCE_DIR}/csrc/generate/main.cpp
  ${CMAKE_SOURCE_DIR}/csrc/shared/chinese_checkers.cpp
)
target_include_directories(generate PUBLIC
  ${CMAKE_SOURCE_DIR}/csrc/shared
  ${TORCH_INCLUDE_DIRS}
)
target_link_libraries(generate "${TORCH_LIBRARIES}")
set_property(TARGET generate PROPERTY CXX_STANDARD 20)