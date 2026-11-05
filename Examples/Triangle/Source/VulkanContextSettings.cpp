#include "VulkanAbstractions/VulkanContext.hpp"

const bool VulkanContext::GraphicsCapabilitiesEnabled = true;

#ifdef NDEBUG
const bool VulkanContext::ValidationLayersEnabled = false;
const bool VulkanContext::EnableInfoLogs = false;
#else
const bool VulkanContext::ValidationLayersEnabled = true;
const bool VulkanContext::EnableInfoLogs = true;
#endif

const std::vector<const char*> VulkanContext::RequiredInstanceExtensions{};
const std::vector<const char*> VulkanContext::RequiredValidationLayers{ "VK_LAYER_KHRONOS_validation" };
const std::vector<const char*> VulkanContext::RequiredDeviceExtensions{ vk::KHRSwapchainExtensionName };

const vk::StructureChain RequiredDeviceFeatures{
  vk::PhysicalDeviceFeatures2{},
  vk::PhysicalDeviceVulkan11Features{
    .shaderDrawParameters = true,
  },
  vk::PhysicalDeviceVulkan13Features{
    .synchronization2 = true,
    .dynamicRendering = true,
  },
};

const vk::PhysicalDeviceFeatures2& VulkanContext::GetRequiredDeviceFeatures() const noexcept {
  return RequiredDeviceFeatures.get<vk::PhysicalDeviceFeatures2>();
}

int VulkanContext::GetPhysicalDeviceScore(const vk::PhysicalDevice& physicalDevice) const noexcept {
  int score = 0;

  const auto features = physicalDevice.getFeatures2<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan11Features,
    vk::PhysicalDeviceVulkan13Features
  >();

  const auto& vk11 = features.get<vk::PhysicalDeviceVulkan11Features>();
  const auto& vk13 = features.get<vk::PhysicalDeviceVulkan13Features>();

  if (!vk11.shaderDrawParameters) {
    return 0;
  }

  if (!vk13.synchronization2) {
    return 0;
  }

  if (!vk13.dynamicRendering) {
    return 0;
  }

  const vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();

  switch (properties.deviceType) {
  case vk::PhysicalDeviceType::eDiscreteGpu:
    score += 1000;
    break;
  case vk::PhysicalDeviceType::eIntegratedGpu:
    score += 500;
    break;
  default:
    score += 100;
  }

  return score;
}
