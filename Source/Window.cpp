#include <stdexcept>
#include <format>
#include <utility>

#include "Window.hpp"

Window::Window(const std::string_view title, const int width, const int height) {
  handle = SDL_CreateWindow(
    title.data(),
    width,
    height,
    SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY
  );

  if (handle == nullptr) {
    throw std::runtime_error{ std::format("Failed to create window because of error: {}", SDL_GetError()) };
  }

  id = SDL_GetWindowID(handle);
}

Window::~Window() noexcept {
  SDL_DestroyWindow(handle);
}

std::expected<std::pair<int, int>, std::string> Window::GetFramebufferSize() const noexcept {
  int width, height;
  if (!SDL_GetWindowSize(handle, &width, &height)) {
    return std::unexpected{ std::format("Failed to get window framebuffer size because of error: {}", SDL_GetError()) };
  }
  return std::pair{ width, height };
}
Window::State Window::GetState() const noexcept {
  const SDL_WindowFlags flags = SDL_GetWindowFlags(handle);

  if (flags & SDL_WINDOW_MINIMIZED) {
    return State::Minimized;
  }

  if (flags & SDL_WINDOW_MAXIMIZED) {
    return State::Maximized;
  }

  return State::Restored;
}
