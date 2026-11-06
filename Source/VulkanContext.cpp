#include <stdexcept>
#include <format>
#include <ranges>
#include <string_view>
#include <print>

#include <SDL3/SDL_vulkan.h>
#include "vulkan/vulkan.hpp"

#include "VulkanContext.hpp"

#ifndef VK_API_VERSION_1_4
#error "Vulkan 1.4 headers are not available"
#endif

VulkanContext::VulkanContext(const vk::ApplicationInfo& applicationInfo) {
  CreateInstance(applicationInfo);
  SelectPhysicalDevice();
  CreateDeviceAndQueue();
}

VulkanContext::~VulkanContext() noexcept {
  device.waitIdle();
}

std::vector<std::string> VulkanContext::GetRequiredSdlInstanceExtensions() const {
  std::uint32_t sdlExtensionCount = 0;
  const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);

  if (!sdlExtensions || sdlExtensionCount == 0) {
    throw std::runtime_error{
      std::format("Failed to get required SDL Vulkan instance extensions with error {}", SDL_GetError())
    };
  }

  std::vector<std::string> requiredExtensions;
  requiredExtensions.reserve(sdlExtensionCount);

  for (std::uint32_t i = 0; i < sdlExtensionCount; ++i) {
    requiredExtensions.emplace_back(sdlExtensions[i]);
  }

  return requiredExtensions;
}

void VulkanContext::CreateInstance(vk::ApplicationInfo applicationInfo) {
  applicationInfo.apiVersion = ApiVersion;

  std::vector<std::string> requiredExtensions;

  if (GraphicsCapabilitiesEnabled) {
    for (const std::string_view extension : GetRequiredSdlInstanceExtensions()) {
      requiredExtensions.emplace_back(extension);
    }
  }

  for (const std::string_view extension : RequiredInstanceExtensions) {
    requiredExtensions.emplace_back(extension);
  }

  if (EnableInfoLogs) {
    std::println("Required instance extensions:");
    for (const std::string_view extension : requiredExtensions) {
      std::println("\t{}", extension);
    }
    std::println(""); // Padding
  }

  const std::vector unsupportedExtensions =
    requiredExtensions |
    std::views::filter(
      [this](const std::string_view extension) -> bool {
        const bool supported = std::ranges::any_of(
          context.enumerateInstanceExtensionProperties(),
          [extension](const vk::ExtensionProperties& supportedExtension) -> bool {
            return extension == supportedExtension.extensionName;
          }
        );
        return !supported;
      }
    ) |
    std::ranges::to<std::vector>();

  if (!unsupportedExtensions.empty()) {
    std::string missingExtensions;
    for (const std::string_view extension : unsupportedExtensions) {
      missingExtensions += '\t';
      missingExtensions += extension;
      missingExtensions += '\n';
    }
    throw std::runtime_error{ std::format("Unsupported instance extensions:\n{}", missingExtensions) };
  }

  if (EnableInfoLogs) {
    std::println("Vulkan validation layers enabled: {}", ValidationLayersEnabled ? "true" : "false");
    std::println(""); // Padding
  }

  if (ValidationLayersEnabled) {
    if (EnableInfoLogs) {
      std::println("Enabled validation layers:");
      for (const std::string_view layer : RequiredValidationLayers) {
        std::println("\t{}", layer);
      }
      std::println(""); // Padding
    }

    const std::vector supportedLayers = context.enumerateInstanceLayerProperties();
    const std::vector unsupportedLayers =
      RequiredValidationLayers |
      std::views::filter(
        [supportedLayers](const std::string_view requiredLayer) -> bool {
          const bool supported = std::ranges::any_of(
            supportedLayers,
            [requiredLayer](const vk::LayerProperties& supportedLayer) -> bool {
              return requiredLayer == supportedLayer.layerName;
            }
          );
          return !supported;
        }
      ) |
      std::ranges::to<std::vector>();

    if (!unsupportedLayers.empty()) {
      std::string missingLayers;
      for (const std::string_view layerName : unsupportedLayers) {
        missingLayers += '\t';
        missingLayers += layerName;
        missingLayers += '\n';
      }

      throw std::runtime_error{ std::format("Unsupported Vulkan validation layers`:\n{}", missingLayers) };
    }
  }

  std::vector<const char*> rawExtensionStrings;
  for (const std::string_view extension : requiredExtensions) {
    rawExtensionStrings.emplace_back(extension.data());
  }

  const vk::InstanceCreateInfo instanceInfo{
    .pApplicationInfo = &applicationInfo,
    .enabledLayerCount = ValidationLayersEnabled ? static_cast<std::uint32_t>(RequiredValidationLayers.size()) : 0,
    .ppEnabledLayerNames = RequiredValidationLayers.data(),
    .enabledExtensionCount = static_cast<std::uint32_t>(rawExtensionStrings.size()),
    .ppEnabledExtensionNames = rawExtensionStrings.data(),
  };

  instance = vk::raii::Instance{ context, instanceInfo };
}

void VulkanContext::SelectPhysicalDevice() {
  if (EnableInfoLogs) {
    std::println("Required device extensions:");
    for (const std::string_view deviceExtension : RequiredDeviceExtensions) {
      std::println("\t{}", deviceExtension);
    }
    std::println(""); // Padding
  }

  std::vector<int> candidateScores;
  const std::vector candidates = instance.enumeratePhysicalDevices();

  for (const vk::raii::PhysicalDevice& candidate : candidates) {
    candidateScores.emplace_back(EvaluatePhysicalDevice(candidate));
  }

  const auto it = std::ranges::max_element(
    candidateScores,
    [](const auto& a, const auto& b) -> bool {
      return a < b;
    }
  );

  if (it == candidateScores.end()) {
    throw std::runtime_error{ "Failed to find suitable physical device" };
  }

  const std::size_t index = std::distance(candidateScores.begin(), it);
  const auto highestScore = candidateScores[index];

  if (highestScore == 0) {
    throw std::runtime_error{ "Failed to find suitable physical device" };
  }

  physicalDevice = std::move(candidates[index]);

  if (EnableInfoLogs) {
    std::println("Selected physical device:");
    std::println("\t{}", physicalDevice.getProperties().deviceName.data());
    std::println(""); // Padding
  }
}

int VulkanContext::EvaluatePhysicalDevice(const vk::PhysicalDevice candidate) const {
  const vk::PhysicalDeviceProperties properties = candidate.getProperties();
  
  if (EnableInfoLogs) {
    const auto& limits = properties.limits;

    std::println("{}", candidate.getProperties().deviceName.data());

    std::println("\tDriver version: {}", properties.driverVersion);
    std::println("\tVendor ID: {}", properties.vendorID);
    std::println("\tDevice type: {}", vk::to_string(properties.deviceType));

    std::println("\n\tImage limits");
    std::println("\tMax image dimension 1D: {}", limits.maxImageDimension1D);
    std::println("\tMax image dimension 2D: {}", limits.maxImageDimension2D);
    std::println("\tMax image dimension 3D: {}", limits.maxImageDimension3D);
    std::println("\tMax image dimension cube: {}", limits.maxImageDimensionCube);
    std::println("\tMax image array layers: {}", limits.maxImageArrayLayers);
    std::println("\tMax texel buffer elements: {}", limits.maxTexelBufferElements);

    std::println("\n\tBuffer limits");
    std::println("\tMax uniform buffer range: {}", limits.maxUniformBufferRange);
    std::println("\tMax storage buffer range: {}", limits.maxStorageBufferRange);

    std::println("\n\tPush constant limits");
    std::println("\tMax push constants size: {}", limits.maxPushConstantsSize);

    std::println("\n\tDescriptor set limits");
    std::println("\tMax bound descriptor sets: {}", limits.maxBoundDescriptorSets);
    std::println("\tMax per-stage descriptor samplers: {}", limits.maxPerStageDescriptorSamplers);
    std::println("\tMax per-stage descriptor uniform buffers: {}", limits.maxPerStageDescriptorUniformBuffers);
    std::println("\tMax per-stage descriptor storage buffers: {}", limits.maxPerStageDescriptorStorageBuffers);
    std::println("\tMax per-stage descriptor sampled images: {}", limits.maxPerStageDescriptorSampledImages);
    std::println("\tMax per-stage descriptor storage images: {}", limits.maxPerStageDescriptorStorageImages);

    std::println("\n\tVertex and fragment limits");
    std::println("\tMax vertex input attributes: {}", limits.maxVertexInputAttributes);
    std::println("\tMax vertex input bindings: {}", limits.maxVertexInputBindings);
    std::println("\tMax vertex output components: {}", limits.maxVertexOutputComponents);
    std::println("\tMax fragment input components: {}", limits.maxFragmentInputComponents);
    std::println("\tMax fragment output attachments: {}", limits.maxFragmentOutputAttachments);
    std::println("\tMax color attachments: {}", limits.maxColorAttachments);

    std::println("\n\tCompute limits");
    std::println("\tMax compute shared memory size: {}", limits.maxComputeSharedMemorySize);
    std::println("\tMax compute work group invocations: {}", limits.maxComputeWorkGroupInvocations);
    std::println("\tMax compute work group size: [{}, {}, {}]", limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupSize[1], limits.maxComputeWorkGroupSize[2]);
    std::println("\tMax compute work group count: [{}, {}, {}]", limits.maxComputeWorkGroupCount[0], limits.maxComputeWorkGroupCount[1], limits.maxComputeWorkGroupCount[2]);
    
    std::println(""); // Padding
  }

  int score = GetPhysicalDeviceScore(candidate);

  if (properties.apiVersion < ApiVersion) {
    std::println("\tDevice does not support Vulkan 1.4");
    score = 0;
  }

  if (GraphicsCapabilitiesEnabled) {
    const bool supportsGraphicsFamily = std::ranges::any_of(
      candidate.getQueueFamilyProperties(),
      [](const vk::QueueFamilyProperties& familyProperties) -> bool {
        return (familyProperties.queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics;
      }
    );

    if (!supportsGraphicsFamily) {
      std::println("\tDevice is missing a graphics capable queue family");
      score = 0;
    }
  }

  for (const std::string_view requiredExtension : RequiredDeviceExtensions) {
    const bool supported = std::ranges::any_of(
      candidate.enumerateDeviceExtensionProperties(),
      [requiredExtension](const vk::ExtensionProperties& extensionProperties) -> bool {
        return requiredExtension == extensionProperties.extensionName;
      }
    );

    if (!supported) {
      std::println("Device is missing support for extension {}", requiredExtension);
      score = 0;
    }
  }

  return score;
}

void VulkanContext::CreateDeviceAndQueue() {
  const std::vector queueFamiliesProperties = physicalDevice.getQueueFamilyProperties();
  std::optional<std::uint32_t> validQueueFamily;

  for (std::size_t i = 0; i < queueFamiliesProperties.size(); ++i) {
    const vk::QueueFamilyProperties properties = queueFamiliesProperties[i];
    const vk::QueueFlags flags = properties.queueFlags;
    const vk::QueueFlagBits requiredQueueFlags =
      GraphicsCapabilitiesEnabled ? vk::QueueFlagBits::eGraphics : vk::QueueFlagBits::eCompute;

    if ((flags & requiredQueueFlags) == requiredQueueFlags) {
      validQueueFamily = i;
      break;
    }
  }

  if (!validQueueFamily) {
    throw std::runtime_error{ "Failed to find valid queue family" };
  }

  queueFamilyIndex = validQueueFamily.value();

  constexpr float queuePriority{ 1.0F };
  const vk::DeviceQueueCreateInfo deviceQueueInfo{
    .queueFamilyIndex = queueFamilyIndex,
    .queueCount = 1,
    .pQueuePriorities = &queuePriority
  };

  const vk::DeviceCreateInfo logicalDeviceInfo{
    .pNext = &GetRequiredDeviceFeatures(),
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &deviceQueueInfo,
    .enabledExtensionCount = static_cast<std::uint32_t>(RequiredDeviceExtensions.size()),
    .ppEnabledExtensionNames = RequiredDeviceExtensions.data()
  };

  device = vk::raii::Device{ physicalDevice, logicalDeviceInfo };
  queue = vk::raii::Queue{ device, queueFamilyIndex, 0 };
}
