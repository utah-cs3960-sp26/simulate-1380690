cmake_minimum_required(VERSION 3.20)
project(physics_sim LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL3 REQUIRED)

add_executable(physics_sim
    src/main.cpp
    src/simulation.cpp
    src/renderer.cpp
)

target_link_libraries(physics_sim PRIVATE SDL3::SDL3)
