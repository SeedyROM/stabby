#pragma once

#include <SDL2/SDL.h>
#include <exception>
#include <memory>
#include <optional>
#include <string>

namespace ste::sdl {

// Error handling
struct Error : std::exception {
  std::string message;
  std::string sdl_error;

  Error(const std::string &msg) : message(msg), sdl_error(SDL_GetError()) {}

  std::string full_message() const {
    return message + "\nSDL Error: " + sdl_error;
  }

  const char *what() const noexcept override { return full_message().c_str(); }
};

// RAII wrapper for SDL subsystems
class Subsystem {
public:
  explicit Subsystem(Uint32 flags) : flags(flags) {
    if (SDL_InitSubSystem(flags) < 0) {
      throw Error{"Failed to initialize SDL subsystem"};
    }
  }

  ~Subsystem() {
    if (flags) {
      SDL_QuitSubSystem(flags);
    }
  }

  // Prevent copying
  Subsystem(const Subsystem &) = delete;
  Subsystem &operator=(const Subsystem &) = delete;

  // Allow moving
  Subsystem(Subsystem &&other) noexcept : flags(other.flags) {
    other.flags = 0;
  }

  Subsystem &operator=(Subsystem &&other) noexcept {
    if (this != &other) {
      if (flags)
        SDL_QuitSubSystem(flags);
      flags = other.flags;
      other.flags = 0;
    }
    return *this;
  }

private:
  Uint32 flags;
};

// RAII wrapper for SDL_Window
class Window {
public:
  Window(const char *title, int x, int y, int w, int h, Uint32 flags)
      : window(SDL_CreateWindow(title, x, y, w, h, flags)) {
    if (!window) {
      throw Error{"Failed to create window"};
    }
  }

  ~Window() {
    if (window) {
      SDL_DestroyWindow(window);
    }
  }

  // Prevent copying
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  // Allow moving
  Window(Window &&other) noexcept : window(other.window) {
    other.window = nullptr;
  }

  Window &operator=(Window &&other) noexcept {
    if (this != &other) {
      if (window)
        SDL_DestroyWindow(window);
      window = other.window;
      other.window = nullptr;
    }
    return *this;
  }

  SDL_Window *get() const { return window; }
  operator SDL_Window *() const { return window; }

private:
  SDL_Window *window;
};

// RAII wrapper for SDL_Renderer
class Renderer {
public:
  Renderer(SDL_Window *window, int driver_index, Uint32 flags)
      : renderer(SDL_CreateRenderer(window, driver_index, flags)) {
    if (!renderer) {
      throw Error{"Failed to create renderer"};
    }
  }

  ~Renderer() {
    if (renderer) {
      SDL_DestroyRenderer(renderer);
    }
  }

  // Prevent copying
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  // Allow moving
  Renderer(Renderer &&other) noexcept : renderer(other.renderer) {
    other.renderer = nullptr;
  }

  Renderer &operator=(Renderer &&other) noexcept {
    if (this != &other) {
      if (renderer)
        SDL_DestroyRenderer(renderer);
      renderer = other.renderer;
      other.renderer = nullptr;
    }
    return *this;
  }

  SDL_Renderer *get() const { return renderer; }
  operator SDL_Renderer *() const { return renderer; }

  // Common renderer operations
  std::optional<Error> clear() {
    if (SDL_RenderClear(renderer) < 0) {
      return Error{"Failed to clear renderer"};
    }
    return std::nullopt;
  }

  void present() { SDL_RenderPresent(renderer); }

  std::optional<Error> set_draw_color(Uint8 r, Uint8 g, Uint8 b, Uint8 a) {
    if (SDL_SetRenderDrawColor(renderer, r, g, b, a) < 0) {
      return Error{"Failed to set draw color"};
    }
    return std::nullopt;
  }

private:
  SDL_Renderer *renderer;
};

// RAII wrapper for SDL_Texture
class Texture {
public:
  Texture(SDL_Renderer *renderer, const char *path) {
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (!surface) {
      throw Error{"Failed to load surface from " + std::string(path)};
    }

    texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (!texture) {
      throw Error{"Failed to create texture from " + std::string(path)};
    }
  }

  ~Texture() {
    if (texture) {
      SDL_DestroyTexture(texture);
    }
  }

  // Prevent copying
  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;

  // Allow moving
  Texture(Texture &&other) noexcept : texture(other.texture) {
    other.texture = nullptr;
  }

  Texture &operator=(Texture &&other) noexcept {
    if (this != &other) {
      if (texture)
        SDL_DestroyTexture(texture);
      texture = other.texture;
      other.texture = nullptr;
    }
    return *this;
  }

  SDL_Texture *get() const { return texture; }
  operator SDL_Texture *() const { return texture; }

private:
  SDL_Texture *texture;
};

} // namespace ste::sdl