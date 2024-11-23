file(GLOB_RECURSE ENGINE_SOURCES "*.cpp")

add_library(engine ${ENGINE_SOURCES})

target_include_directories(engine PUBLIC ./)

target_link_libraries(engine SDL2::SDL2)
target_link_libraries(engine glm::glm)