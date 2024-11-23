#include <iostream>

#include <SDL2/SDL.h>

#include <engine/engine.h>

int main(int argc, char **argv) {
  engine::test();

  // Initialize SDL
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
    return 1;
  }

  // Create a window
  SDL_Window *window =
      SDL_CreateWindow("Hello, SDL2", 100, 100, 640, 480, SDL_WINDOW_SHOWN);

  if (window == nullptr) {
    std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  // Create a renderer
  SDL_Renderer *renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (renderer == nullptr) {
    SDL_DestroyWindow(window);
    std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
    SDL_Quit();
    return 1;
  }

  // Clear the window
  SDL_RenderClear(renderer);

  // Set the drawing color
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);

  // Create a rectangle
  SDL_Rect rect = {220, 140, 200, 200};

  // Fill the rectangle
  SDL_RenderFillRect(renderer, &rect);

  // Display the renderer
  SDL_RenderPresent(renderer);

  // Wait for 5 seconds
  SDL_Delay(5000);

  return 0;
}