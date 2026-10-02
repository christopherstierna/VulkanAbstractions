#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

class VulkanContext;
class Window;

struct SwapchainRecreationInfo {
  std::uint32_t preferredMaxFramesInFlight{ 2 };
  vk::PresentModeKHR preferredPresentMode{ vk::PresentModeKHR::eMailbox };
  vk::CompositeAlphaFlagBitsKHR preferredAlphaComposite{ vk::CompositeAlphaFlagBitsKHR::eOpaque };
};

class Swapchain {
public:

  Swapchain(VulkanContext& vulkanContext, Window& window);
  ~Swapchain() noexcept = default;

  Swapchain(const Swapchain&) = delete;
  Swapchain& operator=(const Swapchain&) = delete;

  Swapchain(Swapchain&&) noexcept = delete;
  Swapchain& operator=(Swapchain&&) noexcept = delete;

  static constexpr std::uint32_t MaxFramesInFlightLimit{ 3 };

private:

  VulkanContext& vulkanContext;
  Window& window;

  void CreateSurface();
  vk::CompositeAlphaFlagBitsKHR SelectAlphaComposite(vk::CompositeAlphaFlagBitsKHR preferredAlphaComposite) const;
  vk::SurfaceFormatKHR SelectSurfaceFormat() const;
  vk::PresentModeKHR SelectPresentMode(vk::PresentModeKHR preferredPresentMode) const;
  vk::Extent2D SelectExtent() const;
  std::uint32_t SelectImageCount(std::uint32_t preferredMaxFramesInFlight) const;

public:

  vk::raii::SurfaceKHR surface{ nullptr };
  vk::SurfaceFormatKHR surfaceFormat{};
  vk::PresentModeKHR presentMode{};
  vk::Extent2D extent{};
  std::uint32_t imageCount{};
  std::uint32_t minimumImageCount{};
  std::uint32_t maxFramesInFlight{};
  vk::raii::SwapchainKHR swapchain{ nullptr };
  std::vector<vk::Image> images;
  std::vector<vk::raii::ImageView> imageViews;
  vk::CompositeAlphaFlagBitsKHR alphaComposite;

  bool isDirty{ true  };

  void Recreate(const SwapchainRecreationInfo& recreationInfo = {});

private:

  void CreateSwapchain(const SwapchainRecreationInfo& recreationInfo = {});

};