#include <cstring>
#include <cstddef>

#include "VulkanBuffer.hpp"

VulkanBuffer::VulkanBuffer(
  vk::Buffer buffer,
  const vk::DeviceSize size,
  void* mappedData,
  VmaAllocator allocator,
  VmaAllocation allocation
)
  :
  buffer{ buffer },
  size{ size },
  mappedData{ mappedData },
  allocator{ allocator },
  allocation{ allocation }
{}

VulkanBuffer::~VulkanBuffer() noexcept {
  if (buffer != nullptr) {
    vmaDestroyBuffer(allocator, buffer, allocation);
  }
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& otherBuffer) noexcept
:
  buffer{ otherBuffer.buffer },
  size{ otherBuffer.size },
  mappedData{ otherBuffer.mappedData },
  allocator{ otherBuffer.allocator },
  allocation{ otherBuffer.allocation }
{
  otherBuffer.buffer = nullptr;
  otherBuffer.size = 0;
  otherBuffer.mappedData = nullptr;
  otherBuffer.allocator = nullptr;
  otherBuffer.allocation = nullptr;
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& otherBuffer) noexcept {
  if (buffer != nullptr) {
    vmaDestroyBuffer(allocator, buffer, allocation);
  }

  buffer = otherBuffer.buffer;
  size = otherBuffer.size;
  mappedData = otherBuffer.mappedData;
  allocator = otherBuffer.allocator;
  allocation = otherBuffer.allocation;

  otherBuffer.buffer = nullptr;
  otherBuffer.size = 0;
  otherBuffer.mappedData = nullptr;
  otherBuffer.allocator = nullptr;
  otherBuffer.allocation = nullptr;

  return *this;
}

VulkanBuffer& VulkanBuffer::operator=(std::nullptr_t) noexcept {
  if (buffer != nullptr) {
    vmaDestroyBuffer(allocator, buffer, allocation);
  }

  buffer = nullptr;
  size = 0;
  mappedData = nullptr;
  allocator = nullptr;
  allocation = nullptr;

  return *this;
}

bool VulkanBuffer::operator==(std::nullptr_t) const noexcept {
  return buffer == nullptr;
}

void VulkanBuffer::Write(const VulkanMemoryWriteConfiguration& writeConfiguration) noexcept {
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
