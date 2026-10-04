#include <cstring>
#include <cstddef>

#include "VulkanImage.hpp"

VulkanImage::VulkanImage(
  vk::Image image,
  void* mappedData,
  const vk::Extent3D extent,
  VmaAllocator allocator,
  VmaAllocation allocation
)
  :
  image{ image },
  mappedData{ mappedData },
  extent{ extent },
  allocator{ allocator },
  allocation{ allocation }
{}

VulkanImage::~VulkanImage() noexcept {
  if (image != nullptr) {
    vmaDestroyImage(allocator, image, allocation);
  }
}

VulkanImage::VulkanImage(VulkanImage&& otherImage) noexcept
  :
  image{ otherImage.image },
  mappedData{ otherImage.mappedData },
  extent{ otherImage.extent },
  allocator{ otherImage.allocator },
  allocation{ otherImage.allocation }
{
  otherImage.image = nullptr;
  otherImage.mappedData = nullptr;
  otherImage.extent = vk::Extent3D{ .width = 0, .height = 0, .depth = 0 };
  otherImage.allocator = nullptr;
  otherImage.allocation = nullptr;
}

VulkanImage& VulkanImage::operator=(VulkanImage&& otherImage) noexcept {
  if (image != nullptr) {
    vmaDestroyImage(allocator, image, allocation);
  }

  image = otherImage.image;
  mappedData = otherImage.mappedData;
  extent = otherImage.extent;
  allocator = otherImage.allocator;
  allocation = otherImage.allocation;

  otherImage.image = nullptr;
  otherImage.mappedData = nullptr;
  otherImage.extent = vk::Extent3D{ .width = 0, .height = 0, .depth = 0 };
  otherImage.allocator = nullptr;
  otherImage.allocation = nullptr;

  return *this;
}

VulkanImage& VulkanImage::operator=(std::nullptr_t) noexcept {
  if (image != nullptr) {
    vmaDestroyImage(allocator, image, allocation);
  }

  image = nullptr;
  mappedData = nullptr;
  extent = vk::Extent3D{ .width = 0, .height = 0, .depth = 0 };
  allocator = nullptr;
  allocation = nullptr;

  return *this;
}

bool VulkanImage::operator==(std::nullptr_t) const noexcept {
  return image == nullptr;
}

void VulkanImage::Write(const VulkanMemoryWriteConfiguration& writeConfiguration) noexcept {
  std::memcpy(
    static_cast<std::byte*>(mappedData) + writeConfiguration.dstOffset,
    static_cast<const std::byte*>(writeConfiguration.data) + writeConfiguration.srcOffset,
    writeConfiguration.size
  );

  switch (writeConfiguration.flushConfiguration) {
  case VulkanMemoryFlushConfiguration::FlushWhole:
    vmaFlushAllocation(
      allocator,
      allocation,
      0,
      vk::WholeSize
    );
    break;

  case VulkanMemoryFlushConfiguration::FlushSpecifiedRange:
    vmaFlushAllocation(
      allocator,
      allocation,
      writeConfiguration.flushOffset,
      writeConfiguration.flushSize
    );
    break;

  default:
    break;
  }
}
