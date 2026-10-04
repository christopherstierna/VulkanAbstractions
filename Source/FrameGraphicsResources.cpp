#include "FrameGraphicsResources.hpp"
#include "VulkanContext.hpp"
#include "VulkanMemoryAllocator.hpp"
#include "Swapchain.hpp"

FrameGraphicsResources::FrameGraphicsResources(VulkanContext& vulkanContext, VulkanMemoryAllocator& vulkanMemoryAllocator)
  :
  vulkanContext{ vulkanContext },
  vulkanMemoryAllocator{ vulkanMemoryAllocator } {
  CreateCommandBuffers();
  CreateSynchronizationResources();
}

void FrameGraphicsResources::CreateCommandBuffers() {
  if (commandPool == nullptr) {
    const vk::CommandPoolCreateInfo commandPoolInfo{
      .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
      .queueFamilyIndex = vulkanContext.queueFamilyIndex
    };

    commandPool = vk::raii::CommandPool{ vulkanContext.device, commandPoolInfo };
  }

  commandBuffers.clear();

  const vk::CommandBufferAllocateInfo allocateInfo{
    .commandPool = commandPool,
    .level = vk::CommandBufferLevel::ePrimary,
    .commandBufferCount = Swapchain::MaxFramesInFlightLimit,
  };

  commandBuffers = vk::raii::CommandBuffers{ vulkanContext.device, allocateInfo };
}

void FrameGraphicsResources::CreateSynchronizationResources() {
  fencesFrameInFlight.clear();
  semaphoresImageAvailable.clear();
  semaphoresRenderFinished.clear();

  for (std::size_t i = 0; i < Swapchain::MaxFramesInFlightLimit + 1; ++i) {
    semaphoresRenderFinished.emplace_back(vulkanContext.device, vk::SemaphoreCreateInfo{});
  }

  for (std::size_t i = 0; i < Swapchain::MaxFramesInFlightLimit; ++i) {
    semaphoresImageAvailable.emplace_back(vulkanContext.device, vk::SemaphoreCreateInfo{});
    fencesFrameInFlight.emplace_back(
      vulkanContext.device,
      vk::FenceCreateInfo{ .flags = vk::FenceCreateFlagBits::eSignaled }
    );
  }
}

void FrameGraphicsResources::CreateColorImages(const vk::Extent2D& extent, const vk::ImageUsageFlags usage) {
  colorImageViews.clear();
  colorImages.clear();

  for (std::uint32_t i = 0; i < maxFramesInFlight; ++i) {
    colorImages.emplace_back(vulkanMemoryAllocator.AllocateImage({
        .imageType = vk::ImageType::e2D,
        .format = ColorImageFormat,
        .extent = vk::Extent3D{
          .width = extent.width,
          .height = extent.height,
          .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .sampleCount = vk::SampleCountFlagBits::e1,
        .usage = usage,
        .tiling = vk::ImageTiling::eOptimal,
        .initialLayout = vk::ImageLayout::eUndefined,
        .memoryAccessMode = VulkanMemoryAccessMode::DeviceLocal
      })
    );

    colorImageViews.emplace_back(
      vulkanContext.device,
      vk::ImageViewCreateInfo{
        .flags = {},
        .image = colorImages.back().image,
        .viewType = vk::ImageViewType::e2D,
        .format = ColorImageFormat,
        .subresourceRange = vk::ImageSubresourceRange{
          .aspectMask = vk::ImageAspectFlagBits::eColor,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      }
    );
  }
}

void FrameGraphicsResources::CreateDepthImages(const vk::Extent2D& extent, const vk::ImageUsageFlags usage) {
  depthImageViews.clear();
  depthImages.clear();

  for (std::uint32_t i = 0; i < maxFramesInFlight; ++i) {
    depthImages.emplace_back(vulkanMemoryAllocator.AllocateImage({
        .imageType = vk::ImageType::e2D,
        .format = DepthImageFormat,
        .extent = vk::Extent3D{
          .width = extent.width,
          .height = extent.height,
          .depth = 1,
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .sampleCount = vk::SampleCountFlagBits::e1,
        .usage = usage,
        .tiling = vk::ImageTiling::eOptimal,
        .initialLayout = vk::ImageLayout::eUndefined,
        .memoryAccessMode = VulkanMemoryAccessMode::DeviceLocal
      })
    );

    depthImageViews.emplace_back(
      vulkanContext.device,
      vk::ImageViewCreateInfo{
        .flags = {},
        .image = depthImages.back().image,
        .viewType = vk::ImageViewType::e2D,
        .format = DepthImageFormat,
        .subresourceRange = vk::ImageSubresourceRange{
          .aspectMask = vk::ImageAspectFlagBits::eDepth,
          .baseMipLevel = 0,
          .levelCount = 1,
          .baseArrayLayer = 0,
          .layerCount = 1,
        },
      }
    );
  }
}

void FrameGraphicsResources::Recreate(const FrameGraphicsResourcesRecreationInfo& recreationInfo) {
  vulkanContext.device.waitIdle();
  frameIndex = 0;
  maxFramesInFlight = recreationInfo.maxFramesInFlight;

  if (recreationInfo.createColorImages) {
    CreateColorImages(recreationInfo.colorImageResolution, recreationInfo.colorImageUsageFlags);
  }

  if (recreationInfo.createDepthImages) {
    CreateDepthImages(recreationInfo.depthImageResolution, recreationInfo.depthImageUsageFlags);
  }
}

void FrameGraphicsResources::IncrementFrameIndex() noexcept {
  frameIndex = (frameIndex + 1) % maxFramesInFlight;
}
