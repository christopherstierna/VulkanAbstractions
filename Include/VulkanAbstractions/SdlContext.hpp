#pragma once

#include <stdexcept>
#include <format>

#include <SDL3/SDL.h>

class SdlContext {
public:

  SdlContext() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      throw std::runtime_error{ std::format("Failed to initialize SDL3 because of error: {}", SDL_GetError()) };
    }
  }

  ~SdlContext() {
    SDL_Quit();
  }

};