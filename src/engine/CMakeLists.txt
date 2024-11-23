file(GLOB_RECURSE ENGINE_SOURCES "*.cpp")

add_library(engine ${ENGINE_SOURCES})

target_include_directories(engine PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(engine SDL2::SDL2)
target_link_libraries(engine glm::glm)
target_link_libraries(engine glad)

if(APPLE)
    target_compile_definitions(engine PRIVATE GL_SILENCE_DEPRECATION)
endif()