#include <stdexcept>
#include <cassert>

#include "VulkanMemoryAllocator.hpp"
#include "VulkanContext.hpp"
#include "VulkanBuffer.hpp"
#include "VulkanImage.hpp"

VulkanMemoryAllocator::VulkanMemoryAllocator(VulkanContext& vulkanContext) {
  const VmaAllocatorCreateInfo allocatorCreateInfo{
    .flags = {},
    .physicalDevice = *vulkanContext.physicalDevice,
    .device = *vulkanContext.device,
    .pVulkanFunctions = nullptr,
    .instance = *vulkanContext.instance,
    .vulkanApiVersion = VulkanContext::ApiVersion,
  };
  vmaCreateAllocator(&allocatorCreateInfo, &allocator);
}

VulkanMemoryAllocator::~VulkanMemoryAllocator() noexcept {
  vmaDestroyAllocator(allocator);
}

VulkanBuffer VulkanMemoryAllocator::AllocateBuffer(const BufferAllocationInfo& bufferAllocationInfo) {
  assert(bufferAllocationInfo.size > 0 && "Cannot allocate vulkan buffer of size 0");

  const VkBufferCreateInfo bufferCreateInfo{
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .size = bufferAllocationInfo.size,
    .usage = static_cast<VkBufferUsageFlags>(bufferAllocationInfo.usage),
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  VmaAllocationCreateInfo allocationCreateInfo{};
  allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

  switch (bufferAllocationInfo.memoryAccessMode) {
  case VulkanMemoryAccessMode::HostSequentialWrite:
    allocationCreateInfo.flags =
      VMA_ALLOCATION_CREATE_MAPPED_BIT |
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    break;

  case VulkanMemoryAccessMode::HostRandomReadAndWrite:
    allocationCreateInfo.flags =
      VMA_ALLOCATION_CREATE_MAPPED_BIT |
      VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    break;

  default:
    allocationCreateInfo.flags = {};
    break;
  }

  VkBuffer buffer;
  VmaAllocation allocation;
  VmaAllocationInfo allocationInfo;

  const VkResult result = vmaCreateBuffer(
    allocator,
    &bufferCreateInfo,
    &allocationCreateInfo,
    &buffer,
    &allocation,
    &allocationInfo
  );

  if (result != VK_SUCCESS) {
    throw std::runtime_error{ "Failed to allocate Vulkan buffer" };
  }

  return VulkanBuffer{
    buffer,
    bufferAllocationInfo.size,
    bufferAllocationInfo.memoryAccessMode == VulkanMemoryAccessMode::DeviceLocal ? nullptr : allocationInfo.pMappedData,
    allocator,
    allocation,
  };
}

VulkanImage VulkanMemoryAllocator::AllocateImage(const ImageAllocationInfo& imageAllocationInfo) {
  const VkImageCreateInfo imageCreateInfo{
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .imageType = static_cast<VkImageType>(imageAllocationInfo.imageType),
    .format = static_cast<VkFormat>(imageAllocationInfo.format),
    .extent = static_cast<VkExtent3D>(imageAllocationInfo.extent),
    .mipLevels = imageAllocationInfo.mipLevels,
    .arrayLayers = imageAllocationInfo.arrayLayers,
    .samples = static_cast<VkSampleCountFlagBits>(imageAllocationInfo.sampleCount),
    .tiling = static_cast<VkImageTiling>(imageAllocationInfo.tiling),
    .usage = static_cast<VkImageUsageFlags>(imageAllocationInfo.usage),
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .initialLayout = static_cast<VkImageLayout>(imageAllocationInfo.initialLayout)
  };

  VmaAllocationCreateInfo allocationCreateInfo{};
  allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

  switch (imageAllocationInfo.memoryAccessMode) {
  case VulkanMemoryAccessMode::HostSequentialWrite:
    allocationCreateInfo.flags =
      VMA_ALLOCATION_CREATE_MAPPED_BIT |
      VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
    break;

  case VulkanMemoryAccessMode::HostRandomReadAndWrite:
    allocationCreateInfo.flags =
      VMA_ALLOCATION_CREATE_MAPPED_BIT |
      VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    break;

  default:
    allocationCreateInfo.flags = {};
    break;
  }

  VkImage image;
  VmaAllocation allocation;
  VmaAllocationInfo allocationInfo;
  allocationInfo.pMappedData = nullptr;

  const VkResult result = vmaCreateImage(
    allocator,
    &imageCreateInfo,
    &allocationCreateInfo,
    &image,
    &allocation,
    &allocationInfo
  );

  if (result != VK_SUCCESS) {
    throw std::runtime_error{ "Failed to allocate buffer" };
  }

  return VulkanImage{
    image,
    imageAllocationInfo.memoryAccessMode == VulkanMemoryAccessMode::DeviceLocal ? nullptr : allocationInfo.pMappedData,
    allocator,
    allocation,
  };
}
