#pragma once

#include <string_view>
#include <string>
#include <expected>

#include <SDL3/SDL.h>

class Window {
public:

  Window(std::string_view title, int width, int height);
  ~Window() noexcept;

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  Window(Window&&) noexcept = delete;
  Window& operator=(Window&&) noexcept = delete;

  SDL_Window* handle{ nullptr };
  SDL_WindowID id{};

  std::expected<std::pair<int, int>, std::string> GetFramebufferSize() const noexcept;

  enum class State {
    Restored,
    Minimized,
    Maximized,
  };

  State GetState() const noexcept;

};