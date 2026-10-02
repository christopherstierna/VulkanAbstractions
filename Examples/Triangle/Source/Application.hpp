#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <optional>

#include <vulkan/vulkan_raii.hpp>

#include <VulkanAbstractions/SdlContext.hpp>
#include <VulkanAbstractions/Window.hpp>
#include <VulkanAbstractions/VulkanContext.hpp>
#include <VulkanAbstractions/VulkanMemoryAllocator.hpp>
#include <VulkanAbstractions/Swapchain.hpp>
#include <VulkanAbstractions/FrameGraphicsResources.hpp>
#include <VulkanAbstractions/VulkanPipeline.hpp>

class Application {
public:

  Application();
  ~Application() noexcept;

  Application(const Application&) = delete;
  Application& operator=(const Application&) = delete;

  Application(Application&&) noexcept = delete;
  Application& operator=(Application&&) noexcept = delete;

  void Run();

private:

  bool running{ false };

  SdlContext sdlContext;
  Window window;
  VulkanContext vulkanContext;
  VulkanMemoryAllocator vulkanMemoryAllocator;
  Swapchain swapchain;
  FrameGraphicsResources frameGraphicsResources;
  std::unordered_map<vk::Format, std::unique_ptr<VulkanPipeline>> trianglePipelines;

  void Render();
  void WaitForFrameFence();
  std::optional<std::uint32_t> GetNextSwapchainImage();
  void MakeSwapchainImageDrawOptimal(std::uint32_t imageIndex);
  void DrawTriangle(std::uint32_t imageIndex);
  VulkanPipeline& GetTrianglePipeline(vk::Format format);
  void MakeSwapchainImagePresentOptimal(std::uint32_t imageIndex);
  void SubmitCommandBuffer(std::uint32_t imageIndex);

};