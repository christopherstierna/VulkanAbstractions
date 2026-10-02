#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>

#include "VulkanMemoryWriteConfiguration.hpp"

class VulkanBuffer {
public:

  VulkanBuffer() = default;

  VulkanBuffer(
    vk::Buffer buffer,
    vk::DeviceSize size,
    void* mappedData,
    VmaAllocator allocator,
    VmaAllocation allocation
  );

  ~VulkanBuffer() noexcept;

  VulkanBuffer(const VulkanBuffer&) = delete;
  VulkanBuffer& operator=(const VulkanBuffer&) = delete;

  VulkanBuffer(VulkanBuffer&& otherBuffer) noexcept;
  VulkanBuffer& operator=(VulkanBuffer&& otherBuffer) noexcept;

  VulkanBuffer& operator=(std::nullptr_t) noexcept;
  bool operator==(std::nullptr_t) const noexcept;

  void Write(const VulkanMemoryWriteConfiguration& writeConfiguration) noexcept;

  vk::Buffer buffer{ nullptr };
  vk::DeviceSize size{ 0 };
  void* mappedData{ nullptr };

private:

  VmaAllocator allocator{ nullptr };
  VmaAllocation allocation{ nullptr };

};
