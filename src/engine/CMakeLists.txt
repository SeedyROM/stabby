file(GLOB_RECURSE ENGINE_SOURCES "*.cpp")

add_library(engine STATIC ${ENGINE_SOURCES})  # Explicitly make it a static library

target_include_directories(engine PUBLIC ${CMAKE_SOURCE_DIR}/src)

# Link libraries statically
target_link_libraries(engine PUBLIC SDL2::SDL2-static)  # Use SDL2-static instead of SDL2
target_link_libraries(engine PUBLIC SDL2_image::SDL2_image)
target_link_libraries(engine PUBLIC glm::glm)
target_link_libraries(engine PUBLIC glad::glad)
target_link_libraries(engine PUBLIC vorbis::vorbis)
target_link_libraries(engine PUBLIC Freetype::Freetype)
target_link_libraries(engine PUBLIC nlohmann_json::nlohmann_json)

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

# Assets path
target_compile_definitions(engine PUBLIC
    ASSET_PATH="${PROJECT_SOURCE_DIR}/assets")