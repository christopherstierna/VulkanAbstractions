#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>

#include "VulkanMemoryWriteConfiguration.hpp"

class VulkanImage {
public:

  VulkanImage() = default;

  VulkanImage(
    vk::Image image,
    void* mappedData,
    VmaAllocator allocator,
    VmaAllocation allocation
  );

  ~VulkanImage() noexcept;

  VulkanImage(const VulkanImage&) = delete;
  VulkanImage& operator=(const VulkanImage&) = delete;

  VulkanImage(VulkanImage&& otherImage) noexcept;
  VulkanImage& operator=(VulkanImage&& otherImage) noexcept;

  VulkanImage& operator=(std::nullptr_t) noexcept;
  bool operator==(std::nullptr_t) const noexcept;

  void Write(const VulkanMemoryWriteConfiguration& writeConfiguration) noexcept;

  vk::Image image{ nullptr };
  void* mappedData{ nullptr };

private:

  VmaAllocator allocator{ nullptr };
  VmaAllocation allocation{ nullptr };

};
