#pragma once

#include <memory>
#include <optional>
#include <string>

#include <SDL2/SDL.h>
#include <glad/glad.h>

namespace ste {

class Window {
public:
  Window(const std::string &title, int width, int height, SDL_Window *window,
         SDL_GLContext glContext);
  ~Window();

  static std::optional<std::unique_ptr<Window>> create(const std::string &title,
                                                       int width, int height);

  inline void setTitle(const std::string &title) {
    m_title = title;
    SDL_SetWindowTitle(m_windowPtr, m_title.c_str());
  }
  inline void setWidth(int width) { setDimensions(width, m_height); }
  inline void setHeight(int height) { setDimensions(m_width, height); }
  inline void setDimensions(int width, int height) {
    m_width = width;
    m_height = height;
    SDL_SetWindowSize(m_windowPtr, m_width, m_height);
  }

  inline const std::string &getTitle() const { return m_title; }
  inline int getWidth() const { return m_width; }
  inline int getHeight() const { return m_height; }

  inline void clear() { glClear(GL_COLOR_BUFFER_BIT); }
  inline void clearColor(float r, float g, float b, float a) {
    clear();
    glClearColor(r, g, b, a);
  }

  inline void swapBuffers() { SDL_GL_SwapWindow(m_windowPtr); }
  void setVSync(bool enable);

protected:
  static void initializeOpenGL();

  SDL_Window *m_windowPtr;
  SDL_GLContext m_glContext;
  std::string m_title;
  int m_width;
  int m_height;
};

} // namespace ste