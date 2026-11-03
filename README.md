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

// Validation layers and info logs.
#ifdef NDEBUG
const bool VulkanContext::ValidationLayersEnabled = false;
const bool VulkanContext::EnableInfoLogs = false;
#else
const bool VulkanContext::ValidationLayersEnabled = true;
const bool VulkanContext::EnableInfoLogs = true;
#endif

// What validation layers and device extensions you want to use.
const std::vector<const char*> VulkanContext::RequiredValidationLayers{ "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> VulkanContext::RequiredDeviceExtensions{ vk::KHRSwapchainExtensionName };

// Structure chain of the required device features.
vk::StructureChain RequiredDeviceFeatures{
  vk::PhysicalDeviceFeatures2{},
  vk::PhysicalDeviceVulkan11Features{
    .shaderDrawParameters = true,
  },
  vk::PhysicalDeviceVulkan13Features{
    .synchronization2 = true,
    .dynamicRendering = true,
  },
};

// Definition of GetRequiredDeviceFeatures() which VulkanContext uses to get your required device features.
vk::PhysicalDeviceFeatures2& VulkanContext::GetRequiredDeviceFeatures() noexcept {
  return RequiredDeviceFeatures.get<vk::PhysicalDeviceFeatures2>();
}

// Definition of IsRequiredDeviceFeaturesSupported() which VulkanContext uses to check if your required device features are supported.
// Ensure this checks support for all your enabled device features in the RequiredDeviceFeatures structure chain.
bool VulkanContext::IsRequiredDeviceFeaturesSupported(const vk::PhysicalDevice& physicalDevice) const noexcept {
  const auto features = physicalDevice.getFeatures2<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan11Features,
    vk::PhysicalDeviceVulkan13Features
  >();

  const auto& vk11 = features.get<vk::PhysicalDeviceVulkan11Features>();
  const auto& vk13 = features.get<vk::PhysicalDeviceVulkan13Features>();

  if (!vk11.shaderDrawParameters) {
    return false;
  }

  if (!vk13.synchronization2) {
    return false;
  }

  if (!vk13.dynamicRendering) {
    return false;
  }

  return true;
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
