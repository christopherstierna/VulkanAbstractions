#pragma once

#include <vector>
#include <string>

#include <vulkan/vulkan_raii.hpp>
#include "vulkan/vulkan.hpp"

class VulkanContext {
public:

  explicit VulkanContext(const vk::ApplicationInfo& applicationInfo = {});
  ~VulkanContext() noexcept;

  VulkanContext(const VulkanContext&) = delete;
  VulkanContext& operator=(const VulkanContext&) = delete;

  VulkanContext(VulkanContext&&) noexcept = delete;
  VulkanContext& operator=(VulkanContext&&) noexcept = delete;

  static constexpr std::uint32_t ApiVersion{ vk::ApiVersion14 };

  // These have to be defined by the user
  static const bool GraphicsCapabilitiesEnabled;
  static const bool ValidationLayersEnabled;
  static const bool EnableInfoLogs;
  static const std::vector<const char*> RequiredInstanceExtensions;
  static const std::vector<const char*> RequiredValidationLayers;
  static const std::vector<const char*> RequiredDeviceExtensions;
  const vk::PhysicalDeviceFeatures2& GetRequiredDeviceFeatures() const noexcept;
  int GetPhysicalDeviceScore(const vk::PhysicalDevice& physicalDevice) const noexcept;

  vk::raii::Context context;
  vk::raii::Instance instance{ nullptr };
  vk::raii::PhysicalDevice physicalDevice{ nullptr };
  vk::raii::Device device{ nullptr };
  vk::raii::Queue queue{ nullptr };
  std::uint32_t queueFamilyIndex{ 0 };

private:

  std::vector<std::string> GetRequiredSdlInstanceExtensions() const;
  void CreateInstance(vk::ApplicationInfo applicationInfo);
  void SelectPhysicalDevice();
  int EvaluatePhysicalDevice(vk::PhysicalDevice candidate) const;
  void CreateDeviceAndQueue();

};