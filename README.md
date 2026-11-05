# VulkanAbstractions

Reusable Vulkan abstractions for my personal C++ applications.

This library exists to avoid repeatedly implementing the same Vulkan setup and boilerplate across projects. It provides a collection of lightweight abstractions around common Vulkan functionality and classes.

## Requirements

* **C++23**
* **Vulkan 1.4** headers
* [Vulkan Memory Allocator (VMA)](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
* [SDL3](https://github.com/libsdl-org/SDL)

## Building

VulkanAbstractions is built as a **static library**.

Build and link to it using CMake:
```cmake
add_subdirectory(SomeDirectory/VulkanAbstractions)
target_link_libraries(YourApplication PRIVATE VulkanAbstractions)
```

Your application also needs to define certain members and methods related to the VulkanContext, here is a minimal example.
```cpp
#include "VulkanAbstractions/VulkanContext.hpp"

const bool VulkanContext::GraphicsCapabilitiesEnabled = true;

#ifdef NDEBUG
const bool VulkanContext::ValidationLayersEnabled = false;
const bool VulkanContext::EnableInfoLogs = false;
#else
const bool VulkanContext::ValidationLayersEnabled = true;
const bool VulkanContext::EnableInfoLogs = true;
#endif

const std::vector<const char*> VulkanContext::RequiredInstanceExtensions{};
const std::vector<const char*> VulkanContext::RequiredValidationLayers{ "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> VulkanContext::RequiredDeviceExtensions{ vk::KHRSwapchainExtensionName };

// Device features you want to use.
const vk::StructureChain RequiredDeviceFeatures{
  vk::PhysicalDeviceFeatures2{},
  vk::PhysicalDeviceVulkan11Features{
    .shaderDrawParameters = true,
  },
  vk::PhysicalDeviceVulkan13Features{
    .synchronization2 = true,
    .dynamicRendering = true,
  },
};

// Return your RequiredDeviceFeatures structure chain.
const vk::PhysicalDeviceFeatures2& VulkanContext::GetRequiredDeviceFeatures() const noexcept {
  return RequiredDeviceFeatures.get<vk::PhysicalDeviceFeatures2>();
}

// This should return a score of zero if a required device feature in your RequiredDeviceFeatures structure chain is unsupported.
int VulkanContext::GetPhysicalDeviceScore(const vk::PhysicalDevice& physicalDevice) const noexcept {
  int score = 0;

  const auto features = physicalDevice.getFeatures2<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan11Features,
    vk::PhysicalDeviceVulkan13Features
  >();

  const auto& vk11 = features.get<vk::PhysicalDeviceVulkan11Features>();
  const auto& vk13 = features.get<vk::PhysicalDeviceVulkan13Features>();

  if (!vk11.shaderDrawParameters) {
    return 0;
  }

  if (!vk13.synchronization2) {
    return 0;
  }

  if (!vk13.dynamicRendering) {
    return 0;
  }

  const vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

  // Example: prefer discrete graphics cards.
  switch (properties.deviceType) {
  case vk::PhysicalDeviceType::eDiscreteGpu:
    score += 1000;
    break;
  case vk::PhysicalDeviceType::eIntegratedGpu:
    score += 500;
    break;
  default:
    score += 100;
  }

  return score;
}

```

Headers can then be included using:

```cpp
#include <VulkanAbstractions/VulkanContext.hpp>
#include <VulkanAbstractions/VulkanMemoryAllocator.hpp>
#include <VulkanAbstractions/VulkanPipeline.hpp>
etc...
```

Replace the header names with the specific components you need.

## Usage

The library is intended to provide reusable building blocks for Vulkan applications, handling common setup and boilerplate while allowing applications to work directly with Vulkan where necessary.

The exact abstractions and APIs are subject to change as the library evolves alongside my personal projects.

## Dependencies

VulkanAbstractions relies on:

* **Vulkan** - graphics API
* **Vulkan Memory Allocator** - GPU memory allocation
* **SDL3** - windowing and platform integration

These dependencies are not intended to be reimplemented by this library; VulkanAbstractions primarily provides a reusable layer around them.

## Status

This is a **personal-use library** and is developed primarily to support my own applications.

The API may change without notice, and documentation does not exist.

## License

VulkanAbstractions is licensed under the [BSD Zero Clause License (0BSD)](LICENSE).
