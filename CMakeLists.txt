cmake_minimum_required(VERSION 3.23)

project(stabby)

# Set the C++ standard to 23
set(CMAKE_CXX_STANDARD 23)

# Load dependencies
set(BUILD_SHARED_LIBS OFF)
set(SDL2_STATIC ON)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(glm REQUIRED)
find_package(glad REQUIRED)
find_package(Vorbis REQUIRED)
find_package(Freetype REQUIRED)

# Add the library/game directories
add_subdirectory(src/engine)
add_subdirectory(src/game)