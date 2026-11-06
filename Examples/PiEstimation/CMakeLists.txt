cmake_minimum_required(VERSION 3.25.0)

project(PiEstimation)

add_executable(
  PiEstimation
  Source/Main.cpp
  Source/VulkanContextSettings.cpp
  Source/EstimatePi.cpp
)

set_target_properties(
  PiEstimation
  PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

target_link_libraries(PiEstimation PRIVATE VulkanAbstractions)

add_custom_command(
  TARGET PiEstimation POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  "${CMAKE_CURRENT_SOURCE_DIR}/Resource"
  "${CMAKE_CURRENT_BINARY_DIR}/Resource"
)