#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "VulkanImage.hpp"
#include "Swapchain.hpp"

class VulkanContext;
class Swapchain;
class VulkanMemoryAllocator;

struct FrameGraphicsResourcesRecreationInfo {
  std::uint32_t maxFramesInFlight{ Swapchain::MaxFramesInFlightLimit };
  bool createColorImages{ false };
  vk::Extent2D colorImageResolution{ .width = 1, .height = 1 };
  bool createDepthImages{ false };
  vk::Extent2D depthImageResolution{ .width = 1, .height = 1 };
};

class FrameGraphicsResources {
public:

  FrameGraphicsResources(
    VulkanContext& vulkanContext,
    VulkanMemoryAllocator& vulkanMemoryAllocator
  );

  ~FrameGraphicsResources() noexcept = default;

  FrameGraphicsResources(const FrameGraphicsResources&) = delete;
  FrameGraphicsResources& operator=(const FrameGraphicsResources&) = delete;

  FrameGraphicsResources(FrameGraphicsResources&&) noexcept = delete;
  FrameGraphicsResources& operator=(FrameGraphicsResources&&) noexcept = delete;

private:

  VulkanContext& vulkanContext;
  VulkanMemoryAllocator& vulkanMemoryAllocator;

  void CreateCommandBuffers();
  void CreateSynchronizationResources();
  void CreateColorImages(const vk::Extent2D& extent);
  void CreateDepthImages(const vk::Extent2D& extent);

public:

  vk::raii::CommandPool commandPool{ nullptr };
  vk::raii::CommandBuffers commandBuffers{ nullptr };

  std::vector<vk::raii::Fence> fencesFrameInFlight;
  std::vector<vk::raii::Semaphore> semaphoresImageAvailable;
  std::vector<vk::raii::Semaphore> semaphoresRenderFinished;

  static constexpr vk::Format ColorImageFormat{ vk::Format::eR8G8B8A8Srgb };
  std::vector<VulkanImage> colorImages;
  std::vector<vk::raii::ImageView> colorImageViews;

  static constexpr vk::Format DepthImageFormat{ vk::Format::eD32Sfloat };
  std::vector<VulkanImage> depthImages;
  std::vector<vk::raii::ImageView> depthImageViews;

  std::uint32_t frameIndex{ 0 };

private:

  std::uint32_t maxFramesInFlight{ Swapchain::MaxFramesInFlightLimit };

public:

  void Recreate(const FrameGraphicsResourcesRecreationInfo& recreationInfo = {});
  void IncrementFrameIndex() noexcept;

};