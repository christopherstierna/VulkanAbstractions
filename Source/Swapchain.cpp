#include <utility>
#include <stdexcept>
#include <format>

#include <SDL3/SDL_vulkan.h>

#include "Swapchain.hpp"
#include "VulkanContext.hpp"
#include "Window.hpp"

Swapchain::Swapchain(VulkanContext& vulkanContext, Window& window)
  :
  vulkanContext{ vulkanContext },
  window{ window } {
  CreateSurface();
}

void Swapchain::CreateSurface() {
  VkSurfaceKHR rawSurface;

  if (!SDL_Vulkan_CreateSurface(window.handle, *vulkanContext.instance, nullptr, &rawSurface)) {
    throw std::runtime_error{ std::format("Failed to create window Vulkan surface with error: {}", SDL_GetError()) };
  }

  surface = vk::raii::SurfaceKHR{ vulkanContext.instance, rawSurface };
}

vk::CompositeAlphaFlagBitsKHR Swapchain::SelectAlphaComposite(const vk::CompositeAlphaFlagBitsKHR preferredAlphaComposite) const {
  const auto capabilities = vulkanContext.physicalDevice.getSurfaceCapabilitiesKHR(surface);

  if ((preferredAlphaComposite & capabilities.supportedCompositeAlpha) == vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) {
    return preferredAlphaComposite;
  }

  return vk::CompositeAlphaFlagBitsKHR::eOpaque;
}

vk::SurfaceFormatKHR Swapchain::SelectSurfaceFormat() const {
  const std::vector availableFormats = vulkanContext.physicalDevice.getSurfaceFormatsKHR(surface);

  if (availableFormats.size() == 0) {
    throw std::runtime_error{ "Failed to select a surface format" };
  }

  for (const vk::SurfaceFormatKHR& format : availableFormats) {
    if (format.format == vk::Format::eB8G8R8A8Srgb &&
        format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return format;
    }

    if (format.format == vk::Format::eR8G8B8A8Srgb &&
        format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
      return format;
    }
  }

  return availableFormats[0];
}

vk::PresentModeKHR Swapchain::SelectPresentMode(const vk::PresentModeKHR preferredPresentMode) const {
  const std::vector availableModes = vulkanContext.physicalDevice.getSurfacePresentModesKHR(surface);

  const bool supportsPreferredMode = std::ranges::any_of(
    availableModes,
    [&preferredPresentMode](const vk::PresentModeKHR mode) -> bool {
      return mode == preferredPresentMode;
    }
  );

  if (supportsPreferredMode) {
    return preferredPresentMode;
  }

  return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::SelectExtent() const {
  const vk::SurfaceCapabilitiesKHR surfaceCapabilities = vulkanContext.physicalDevice.getSurfaceCapabilitiesKHR(surface);

  if (surfaceCapabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
    return surfaceCapabilities.currentExtent;
  }

  const auto [width, height] = *window.GetFramebufferSize();

  return vk::Extent2D{
    .width = std::clamp<std::uint32_t>(
      width,
      surfaceCapabilities.minImageExtent.width,
      surfaceCapabilities.maxImageExtent.width
    ),

    .height = std::clamp<std::uint32_t>(
      height,
      surfaceCapabilities.minImageExtent.height,
      surfaceCapabilities.maxImageExtent.height
    )
  };
}

std::uint32_t Swapchain::SelectImageCount(const std::uint32_t preferredMaxFramesInFlight) const {
  const vk::SurfaceCapabilitiesKHR surfaceCapabilities = vulkanContext.physicalDevice.getSurfaceCapabilitiesKHR(surface);

  const std::uint32_t minimum = surfaceCapabilities.minImageCount;
  const std::uint32_t maximum = surfaceCapabilities.maxImageCount;

  if (maximum == 0) {
    return preferredMaxFramesInFlight + 1;
  }

  return std::max(std::min(preferredMaxFramesInFlight + 1, maximum), minimum);
}

void Swapchain::CreateSwapchain(const SwapchainRecreationInfo& recreationInfo) {
  surfaceFormat = SelectSurfaceFormat();
  presentMode = SelectPresentMode(recreationInfo.preferredPresentMode);
  extent = SelectExtent();
  imageCount = SelectImageCount(std::clamp(recreationInfo.preferredMaxFramesInFlight, 0u, MaxFramesInFlightLimit));
  alphaComposite = SelectAlphaComposite(recreationInfo.preferredAlphaComposite);
  maxFramesInFlight = imageCount - 1;

  const vk::SurfaceCapabilitiesKHR surfaceCapabilities = vulkanContext.physicalDevice.getSurfaceCapabilitiesKHR(surface);

  const vk::SwapchainCreateInfoKHR swapchainInfo{
    .surface = surface,
    .minImageCount = imageCount,
    .imageFormat = surfaceFormat.format,
    .imageColorSpace = surfaceFormat.colorSpace,
    .imageExtent = extent,
    .imageArrayLayers = 1,
    .imageUsage = recreationInfo.imageUsageFlags,
    .imageSharingMode = vk::SharingMode::eExclusive,
    .preTransform = surfaceCapabilities.currentTransform,
    .compositeAlpha = alphaComposite,
    .presentMode = presentMode,
    .clipped = true,
    .oldSwapchain = *swapchain,
  };

  imageViews.clear();
  images.clear();

  swapchain = vk::raii::SwapchainKHR{ vulkanContext.device, swapchainInfo };
  images = swapchain.getImages();

  vk::ImageViewCreateInfo imageViewInfo{
    .viewType = vk::ImageViewType::e2D,
    .format = surfaceFormat.format,
    .subresourceRange = {
      .aspectMask = vk::ImageAspectFlagBits::eColor,
      .levelCount = 1,
      .layerCount = 1
    }
  };

  for (const vk::Image& image : images) {
    imageViewInfo.image = image;
    imageViews.emplace_back(vulkanContext.device, imageViewInfo);
  }
}

void Swapchain::Recreate(const SwapchainRecreationInfo& recreationInfo) {
  vulkanContext.device.waitIdle();
  CreateSwapchain(recreationInfo);
  isDirty = false;
}
