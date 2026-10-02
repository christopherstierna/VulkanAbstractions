cmake_minimum_required(VERSION 3.25.0)

project(VulkanAbstractionsExampleTriangle)

add_executable(
  VulkanAbstractionsExampleTriangle
  Source/Main.cpp
  Source/Application.cpp
  Source/Application.hpp
)

set_target_properties(
  VulkanAbstractionsExampleTriangle
  PROPERTIES
  CXX_STANDARD 23
  CXX_STANDARD_REQUIRED ON
  CXX_EXTENSIONS OFF
)

target_link_libraries(VulkanAbstractionsExampleTriangle PRIVATE VulkanAbstractions)

add_custom_command(
    TARGET VulkanAbstractionsExampleTriangle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_CURRENT_SOURCE_DIR}/Resource"
    "${CMAKE_CURRENT_BINARY_DIR}/Resource"
)