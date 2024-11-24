#include "window.h"
#include <iostream>

namespace ste {

std::atomic<int> Window::GLOBAL_WINDOW_COUNT{0};

// Builder implementation
Window::Builder &Window::Builder::setTitle(std::string title) {
  m_config.title = std::move(title);
  return *this;
}

Window::Builder &Window::Builder::setSize(int width, int height) {
  m_config.width = width;
  m_config.height = height;
  return *this;
}

Window::Builder &Window::Builder::setVSync(bool enable) {
  m_config.vsync = enable;
  return *this;
}

Window::Builder &Window::Builder::setGLVersion(int major, int minor) {
  m_config.glMajor = major;
  m_config.glMinor = minor;
  return *this;
}

Window::Builder &Window::Builder::setResizable(bool resizable) {
  m_config.resizable = resizable;
  return *this;
}

Window::Builder &Window::Builder::setFullscreen(bool fullscreen) {
  m_config.fullscreen = fullscreen;
  return *this;
}

Window::Builder &Window::Builder::setMSAA(int samples) {
  m_config.msaa = samples;
  return *this;
}

std::optional<std::unique_ptr<Window>> Window::Builder::build() {
  if (!Window::initializeSDL()) {
    std::cerr << "Failed to initialize SDL: " << Window::getLastError()
              << std::endl;
    return std::nullopt;
  }

  if (!Window::initializeOpenGL(m_config)) {
    std::cerr << "Failed to initialize OpenGL attributes" << std::endl;
    return std::nullopt;
  }

  Uint32 flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
  if (m_config.resizable)
    flags |= SDL_WINDOW_RESIZABLE;
  if (m_config.fullscreen)
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

  SDL_Window *window = SDL_CreateWindow(
      m_config.title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      m_config.width, m_config.height, flags);

  if (!window) {
    std::cerr << "Failed to create window: " << Window::getLastError()
              << std::endl;
    return std::nullopt;
  }

  SDL_GLContext glContext = SDL_GL_CreateContext(window);
  if (!glContext) {
    std::cerr << "Failed to create OpenGL context: " << Window::getLastError()
              << std::endl;
    Window::cleanup(window, nullptr);
    return std::nullopt;
  }

  if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
    std::cerr << "Failed to initialize GLAD" << std::endl;
    Window::cleanup(window, glContext);
    return std::nullopt;
  }

  // Create window using unique_ptr constructor directly
  auto newWindow =
      std::unique_ptr<Window>(new Window(m_config, window, glContext));
  newWindow->setVSync(m_config.vsync);
  ++GLOBAL_WINDOW_COUNT;

  return newWindow;
}

Window::Window(const Config &config, SDL_Window *window,
               SDL_GLContext glContext)
    : m_config(config), m_windowPtr(window), m_glContext(glContext) {}

Window::~Window() {
  cleanup(m_windowPtr, m_glContext);
  if (--GLOBAL_WINDOW_COUNT == 0) {
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
  }
}

bool Window::initializeSDL() {
  static bool initialized = false;
  if (!initialized) {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO) != 0) {
      return false;
    }
    initialized = true;
  }
  return true;
}

bool Window::initializeOpenGL(const Config &config) {
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, config.glMajor) < 0)
    return false;
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, config.glMinor) < 0)
    return false;
  if (SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                          SDL_GL_CONTEXT_PROFILE_CORE) < 0)
    return false;
  if (SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1) < 0)
    return false;
  if (SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24) < 0)
    return false;

  if (config.msaa > 0) {
    if (SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1) < 0)
      return false;
    if (SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, config.msaa) < 0)
      return false;
  }

  return true;
}

std::string Window::getLastError() { return SDL_GetError(); }

void Window::cleanup(SDL_Window *window, SDL_GLContext context) {
  if (context)
    SDL_GL_DeleteContext(context);
  if (window)
    SDL_DestroyWindow(window);
}

void Window::setVSync(bool enable) {
  m_config.vsync = enable;
  SDL_GL_SetSwapInterval(enable ? 1 : 0);
}

} // namespace ste