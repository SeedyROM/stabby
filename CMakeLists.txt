cmake_minimum_required(VERSION 3.23)

project(stabby)

set(CMAKE_CXX_STANDARD 23)

find_package(SDL2 REQUIRED)
find_package(glm REQUIRED)

add_subdirectory(src/engine)
add_subdirectory(src/game)