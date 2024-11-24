#include "window.h"

#include <glad/glad.h>
#include <iostream>

namespace ste {

static int GLOBAL_WINDOW_COUNT = 0;

Window::Window(const std::string &title, int width, int height,
               SDL_Window *window, SDL_GLContext glContext)
    : m_title(title), m_width(width), m_height(height), m_windowPtr(window),
      m_glContext(glContext) {}

Window::~Window() {
  SDL_DestroyWindow(m_windowPtr);
  SDL_GL_DeleteContext(m_glContext);

  // Quit SDL video subsystem if this is the last window
  if (--GLOBAL_WINDOW_COUNT == 0) {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
  }
}

std::optional<std::unique_ptr<Window>> Window::create(const std::string &title,
                                                      int width, int height) {
  // Initialize SDL video subsystem
  if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
    std::cerr << "Failed to initialize SDL video: " << SDL_GetError()
              << std::endl;
    return std::nullopt;
  }

  // Initialize OpenGL
  initializeOpenGL();

  // Create an SDL2 window with OpenGL context
  SDL_Window *window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, width, height,
                                        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

  if (!window) {
    std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
    return std::nullopt;
  }

  // Create OpenGL context
  SDL_GLContext glContext = SDL_GL_CreateContext(window);
  if (!glContext) {
    std::cerr << "OpenGL context creation failed: " << SDL_GetError()
              << std::endl;
    SDL_DestroyWindow(window);
    SDL_Quit();
    return std::nullopt;
  }

  // Initialize GLAD
  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return std::nullopt;
  }

  // Create a unique_ptr to the window
  auto newWindow =
      std::make_unique<Window>(title, width, height, window, glContext);

  // Increment global window count
  GLOBAL_WINDOW_COUNT++;

  return newWindow;
}

void Window::initializeOpenGL() {
  // Set the OpenGL context version (4.1)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
}

void Window::setVSync(bool enable) {
  if (enable) {
    SDL_GL_SetSwapInterval(1);
  } else {
    SDL_GL_SetSwapInterval(0);
  }
}

} // namespace ste