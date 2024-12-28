#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <SDL2/SDL.h>
#include <glad/glad.h>

namespace ste {

class Window {
public:
  struct Config {
    std::string title = "Untitled Window";
    int width = 800;
    int height = 600;
    bool vsync = false;
    int glMajor = 4;
    int glMinor = 1;
    bool resizable = false;
    bool fullscreen = false;
    int msaa = 0;
    bool allowHighDPI = true;
  };

  class Builder {
  public:
    Builder() = default;
    Builder &setTitle(std::string title);
    Builder &setSize(int width, int height);
    Builder &setVSync(bool enable);
    Builder &setGLVersion(int major, int minor);
    Builder &setResizable(bool resizable);
    Builder &setFullscreen(bool fullscreen);
    Builder &setMSAA(int samples);
    std::optional<std::shared_ptr<Window>> build();

  private:
    Config m_config;
  };

  static Builder builder() { return Builder(); }

  static std::optional<std::shared_ptr<Window>> create(const std::string &title,
                                                       int width, int height) {
    return builder().setTitle(title).setSize(width, height).build();
  }

  ~Window();

  // Delete copy operations
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  // Inline window operations
  inline void setTitle(const std::string &title) {
    m_config.title = title;
    SDL_SetWindowTitle(m_windowPtr, title.c_str());
  }

  inline void setWidth(int width) { setDimensions(width, m_config.height); }
  inline void setHeight(int height) { setDimensions(m_config.width, height); }

  inline void setDimensions(int width, int height) {
    m_config.width = width;
    m_config.height = height;
    SDL_SetWindowSize(m_windowPtr, width, height);
  }

  // Inline getters
  inline const std::string &getTitle() const { return m_config.title; }
  inline int getWidth() const { return m_config.width; }
  inline int getHeight() const { return m_config.height; }
  inline std::pair<int, int> getSize() const {
    return {m_config.width, m_config.height};
  }
  inline static float getDpi() {
    float dpi;

    if (SDL_GetDisplayDPI(0, nullptr, &dpi, nullptr) != 0) {
      return 96.0f;
    }

    return dpi;
  }

  // Inline rendering operations
  inline void clear() { glClear(GL_COLOR_BUFFER_BIT); }

  inline void clearColor(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    clear();
  }

  inline void swapBuffers() { SDL_GL_SwapWindow(m_windowPtr); }

  // Window state queries
  inline bool isMinimized() const {
    return SDL_GetWindowFlags(m_windowPtr) & SDL_WINDOW_MINIMIZED;
  }

  inline bool isFocused() const {
    return SDL_GetWindowFlags(m_windowPtr) & SDL_WINDOW_INPUT_FOCUS;
  }

  inline bool isVSync() const { return m_config.vsync; }

  void setVSync(bool enable);

  inline SDL_Window *getSDLWindow() const { return m_windowPtr; }
  inline SDL_GLContext getGLContext() const { return m_glContext; }

protected:
  // Move constructor to protected section and make Builder a friend
  friend class Builder;
  Window(const Config &config, SDL_Window *window, SDL_GLContext glContext);

private:
  static bool initializeSDL();
  static bool initializeOpenGL(const Config &config);
  static std::string getLastError();
  static void cleanup(SDL_Window *window, SDL_GLContext context);

  SDL_Window *m_windowPtr;
  SDL_GLContext m_glContext;
  Config m_config;
  static std::atomic<int> GLOBAL_WINDOW_COUNT;
};

} // namespace ste