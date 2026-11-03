#include "VulkanAbstractions/VulkanContext.hpp"
#include "vulkan/vulkan.hpp"

#ifdef NDEBUG
const bool VulkanContext::ValidationLayersEnabled = false;
const bool VulkanContext::EnableInfoLogs = false;
#else
const bool VulkanContext::ValidationLayersEnabled = true;
const bool VulkanContext::EnableInfoLogs = true;
#endif

const std::vector<const char*> VulkanContext::RequiredValidationLayers{ "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> VulkanContext::RequiredDeviceExtensions{ vk::KHRSwapchainExtensionName };

vk::StructureChain RequiredDeviceFeatures{
  vk::PhysicalDeviceFeatures2{},
  vk::PhysicalDeviceVulkan11Features{
    .shaderDrawParameters = true,
  },
  vk::PhysicalDeviceVulkan13Features{
    .synchronization2 = true,
    .dynamicRendering = true,
  },
};

vk::PhysicalDeviceFeatures2& VulkanContext::GetRequiredDeviceFeatures() noexcept {
  return RequiredDeviceFeatures.get<vk::PhysicalDeviceFeatures2>();
}

bool VulkanContext::IsRequiredDeviceFeaturesSupported(const vk::PhysicalDevice& physicalDevice) const noexcept {
  const auto features = physicalDevice.getFeatures2<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan11Features,
    vk::PhysicalDeviceVulkan13Features
  >();

  const auto& vk11 = features.get<vk::PhysicalDeviceVulkan11Features>();
  const auto& vk13 = features.get<vk::PhysicalDeviceVulkan13Features>();

  if (!vk11.shaderDrawParameters) {
    return false;
  }

  if (!vk13.synchronization2) {
    return false;
  }

  if (!vk13.dynamicRendering) {
    return false;
  }

  return true;
}