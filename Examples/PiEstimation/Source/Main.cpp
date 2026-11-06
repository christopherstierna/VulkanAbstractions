#include <cstdlib>
#include <print>
#include <exception>
#include <iostream>

void EstimatePi();

int main() {
  try {
    EstimatePi();
  } catch (const std::exception& exception) {
    std::println(std::cerr, "Exception caught: {}", exception.what());
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}