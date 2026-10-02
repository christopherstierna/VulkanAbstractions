#pragma once

#include <cstdint>

enum class VulkanMemoryFlushConfiguration {
  DontFlush,
  FlushWhole,
  FlushSpecifiedRange,
};

struct VulkanMemoryWriteConfiguration {
  const void* data;
  std::uint64_t size;
  std::uint64_t dstOffset{ 0 };
  std::uint64_t srcOffset{ 0 };
  VulkanMemoryFlushConfiguration flushConfiguration{ VulkanMemoryFlushConfiguration::FlushWhole };
  std::uint64_t flushOffset{ 0 };
  std::uint64_t flushSize{ 0 };
};
