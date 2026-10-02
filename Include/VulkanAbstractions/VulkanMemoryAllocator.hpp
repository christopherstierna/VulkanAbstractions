#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vk_mem_alloc.h>

class VulkanContext;
class VulkanBuffer;
class VulkanImage;

enum class VulkanMemoryAccessMode {
  DeviceLocal,
  HostSequentialWrite,
  HostRandomReadAndWrite,
};

class VulkanMemoryAllocator {
public:

  explicit VulkanMemoryAllocator(VulkanContext& vulkanContext);
  ~VulkanMemoryAllocator() noexcept;

  VulkanMemoryAllocator(const VulkanMemoryAllocator&) = delete;
  VulkanMemoryAllocator& operator=(const VulkanMemoryAllocator&) = delete;

  VulkanMemoryAllocator(VulkanMemoryAllocator&&) noexcept = delete;
  VulkanMemoryAllocator& operator=(VulkanMemoryAllocator&&) noexcept = delete;

  struct BufferAllocationInfo {
    vk::DeviceSize size;
    vk::BufferUsageFlags usage;
    VulkanMemoryAccessMode memoryAccessMode{ VulkanMemoryAccessMode::DeviceLocal };
  };

  VulkanBuffer AllocateBuffer(const BufferAllocationInfo& bufferAllocationInfo);

  struct ImageAllocationInfo {
    vk::ImageType imageType;
    vk::Format format;
    vk::Extent3D extent;
    std::uint32_t mipLevels;
    std::uint32_t arrayLayers;
    vk::SampleCountFlagBits sampleCount;
    vk::ImageUsageFlags usage;
    vk::ImageTiling tiling;
    vk::ImageLayout initialLayout;
    VulkanMemoryAccessMode memoryAccessMode{ VulkanMemoryAccessMode::DeviceLocal };
  };

  VulkanImage AllocateImage(const ImageAllocationInfo& imageAllocationInfo);

private:

  VmaAllocator allocator{ nullptr };

};
