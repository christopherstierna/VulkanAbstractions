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

After building the library, link it into your application. The library headers should be made available to any CMake targets linking to it, otherwise add the [include directory](Include) to your project.

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
