file(GLOB_RECURSE EDITOR_SOURCES "*.cpp")

include_directories(./)

add_executable(editor ${EDITOR_SOURCES})

target_link_libraries(editor PUBLIC engine)
target_include_directories(editor PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Ignore deprecation warnings on macOS
if(APPLE)
    target_compile_definitions(editor PRIVATE GL_SILENCE_DEPRECATION)
endif()
