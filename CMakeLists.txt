cmake_minimum_required(VERSION 3.23)

project(stabby)

set(CMAKE_CXX_STANDARD 23)

set(BUILD_SHARED_LIBS OFF)
set(SDL2_STATIC ON)
find_package(SDL2 REQUIRED)

find_package(glm REQUIRED)
find_package(glad REQUIRED)

add_subdirectory(src/engine)
add_subdirectory(src/game)