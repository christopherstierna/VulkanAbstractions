cmake_minimum_required(VERSION 3.25.0)

project(VulkanAbstractions)

set(SOURCE_DIR "Source")
set(INCLUDE_DIR "Include/VulkanAbstractions")

add_library(
    VulkanAbstractions
    STATIC

    "${INCLUDE_DIR}/SdlContext.hpp"

    "${INCLUDE_DIR}/Window.hpp"
    "${SOURCE_DIR}/Window.cpp"

    "${INCLUDE_DIR}/VulkanContext.hpp"
    "${SOURCE_DIR}/VulkanContext.cpp"

    "${SOURCE_DIR}/VmaImplementation.cpp"
    "${INCLUDE_DIR}/VulkanMemoryAllocator.hpp"
    "${SOURCE_DIR}/VulkanMemoryAllocator.cpp"
    "${INCLUDE_DIR}/VulkanMemoryWriteConfiguration.hpp"

    "${SOURCE_DIR}/VulkanBuffer.cpp"
    "${INCLUDE_DIR}/VulkanBuffer.hpp"
    "${SOURCE_DIR}/VulkanImage.cpp"
    "${INCLUDE_DIR}/VulkanImage.hpp"

    "${INCLUDE_DIR}/ReadFile.hpp"

    "${SOURCE_DIR}/VulkanPipeline.cpp"
    "${INCLUDE_DIR}/VulkanPipeline.hpp"
    "${INCLUDE_DIR}/VulkanPipelineOptions.hpp"

    "${SOURCE_DIR}/Swapchain.cpp"
    "${INCLUDE_DIR}/Swapchain.hpp"

    "${SOURCE_DIR}/FrameGraphicsResources.cpp"
    "${INCLUDE_DIR}/FrameGraphicsResources.hpp"
)

set_target_properties(
    VulkanAbstractions
    PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

target_include_directories(
    VulkanAbstractions
    PRIVATE
    Source
    Include/VulkanAbstractions
    INTERFACE
    Include
)

target_compile_definitions(
    VulkanAbstractions
    PUBLIC
    VULKAN_HPP_HANDLE_ERROR_OUT_OF_DATE_AS_SUCCESS
    VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
)

find_package(SDL3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)

target_link_libraries(
    VulkanAbstractions
    PUBLIC
    SDL3::SDL3
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
)

set(BUILD_VULKAN_ABSTRACTIONS_EXAMPLES ON CACHE BOOL "Build VulkanAbstractions examples")

if (BUILD_VULKAN_ABSTRACTIONS_EXAMPLES)
    add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Examples/Triangle")
endif()
