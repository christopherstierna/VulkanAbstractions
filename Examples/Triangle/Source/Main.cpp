#include <cstdlib>
#include <print>
#include <iostream>
#include <exception>

#include "Application.hpp"

int main() {
  try {
    Application application;
    application.Run();
  } catch (const std::exception& exception) {
    std::println(std::cerr, "{}", exception.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}