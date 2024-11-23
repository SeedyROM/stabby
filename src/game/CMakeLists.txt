file(GLOB_RECURSE GAME_SOURCES "*.cpp")

include_directories(./)

add_executable(${PROJECT_NAME} ${GAME_SOURCES})

target_link_libraries(${PROJECT_NAME} engine)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} SDL2::SDL2)
target_link_libraries(${PROJECT_NAME} glm::glm)