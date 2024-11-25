file(GLOB_RECURSE ENGINE_SOURCES "*.cpp")

add_library(engine STATIC ${ENGINE_SOURCES})  # Explicitly make it a static library

target_include_directories(engine PUBLIC ${CMAKE_SOURCE_DIR}/src)

# Link libraries statically
target_link_libraries(engine PRIVATE SDL2::SDL2-static)  # Use SDL2-static instead of SDL2
target_link_libraries(engine PRIVATE glm::glm)
target_link_libraries(engine PRIVATE glad::glad)

# For macOS
if(APPLE)
    target_compile_definitions(engine PRIVATE GL_SILENCE_DEPRECATION)
    
    # On macOS, SDL2 static build needs these frameworks
    target_link_libraries(engine PRIVATE
        "-framework Cocoa"
        "-framework Carbon"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework CoreAudio"
        "-framework AudioToolbox"
        "-framework ForceFeedback"
        "-framework Metal"
    )
endif()

# For Windows, you might need additional dependencies
if(WIN32)
    target_link_libraries(engine PRIVATE
        setupapi.lib
        winmm.lib
        imm32.lib
        version.lib
    )
endif()

# Release flags
if(MSVC)
    # Windows/MSVC
    string(REPLACE "/O2" "/O2 /Zi" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
else()
    # Linux/Mac (gcc/clang)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -g -O3")
endif()