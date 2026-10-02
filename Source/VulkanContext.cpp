#include <stdexcept>
#include <format>
#include <ranges>
#include <string_view>
#include <print>

#include <SDL3/SDL_vulkan.h>

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

  const std::vector supportedExtensions = context.enumerateInstanceExtensionProperties();
  const std::vector requiredSdlExtensions = GetRequiredSdlInstanceExtensions();

  if (EnableInfoLogs) {
    std::println("Required SDL Vulkan instance extensions:");
    for (const std::string_view extension : requiredSdlExtensions) {
      std::println("\t{}", extension);
    }
    std::println(""); // Padding
  }

  const std::vector unsupportedSdlExtensions =
    requiredSdlExtensions |
    std::views::filter(
      [&supportedExtensions](const std::string_view extension) -> bool {
        const bool supported = std::ranges::any_of(
          supportedExtensions,
          [extension](const vk::ExtensionProperties& supportedExtension) -> bool {
            return extension == supportedExtension.extensionName;
          }
        );
        return !supported;
      }
    ) |
    std::ranges::to<std::vector>();

  if (!unsupportedSdlExtensions.empty()) {
    std::string missingExtensions;
    for (const std::string_view extension : unsupportedSdlExtensions) {
      missingExtensions += '\t';
      missingExtensions += extension;
      missingExtensions += '\n';
    }

    throw std::runtime_error{ std::format("Unsupported SDL Vulkan instance extensions:\n{}", missingExtensions) };
  }

  if (ValidationLayersEnabled) {
    if (EnableInfoLogs) {
      std::println("Vulkan validation layers enabled: {}", ValidationLayersEnabled ? "true" : "false");
      std::println(""); // Padding

      std::println("Enabled validationLayers:");
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

  std::vector<const char*> rawSdlExtensions;
  rawSdlExtensions.reserve(requiredSdlExtensions.size());
  for (const std::string_view extension : requiredSdlExtensions) {
    rawSdlExtensions.emplace_back(extension.data());
  }

  const vk::InstanceCreateInfo instanceInfo{
    .pApplicationInfo = &applicationInfo,
    .enabledLayerCount = ValidationLayersEnabled ? RequiredValidationLayers.size() : 0,
    .ppEnabledLayerNames = RequiredValidationLayers.data(),
    .enabledExtensionCount = static_cast<std::uint32_t>(requiredSdlExtensions.size()),
    .ppEnabledExtensionNames = rawSdlExtensions.data(),
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

  std::vector<PhysicalDeviceCandidateResult> candidateResults;
  const std::vector candidates = instance.enumeratePhysicalDevices();

  for (const vk::raii::PhysicalDevice& candidate : candidates) {
    candidateResults.emplace_back(EvaluatePhysicalDevice(candidate));

    if (EnableInfoLogs) {
      const auto& candidateResult = candidateResults.back();

      std::println("Physical device candidate:");
      std::println("\t{}", candidateResult.deviceName);
      std::println("\tScore: {}", candidateResult.GetScore());

      for (const std::string_view deviceInfo : candidateResult.deviceInfos) {
        std::println("\t{}", deviceInfo);
      }

      std::println(""); // Padding

      std::println("\tInadequacies");
      if (candidateResult.GetInvalidationReasons().size() == 0) {
        std::println("\tNone");
      } else {
        for (const std::string_view inadequacy : candidateResult.GetInvalidationReasons()) {
          std::println("\t{}", inadequacy);
        }
      }

      std::println(""); // Padding
    }
  }

  const auto it = std::ranges::max_element(
    candidateResults,
    [](const auto& a, const auto& b) -> bool {
      return a.GetScore() < b.GetScore();
    }
  );

  if (it == candidateResults.end()) {
    throw std::runtime_error{ "Failed to find suitable physical device" };
  }

  const std::size_t index = std::distance(candidateResults.begin(), it);
  const auto result = candidateResults[index];

  if (result.GetScore() == 0) {
    throw std::runtime_error{ "Failed to find suitable physical device" };
  }

  physicalDevice = std::move(candidates[index]);

  if (EnableInfoLogs) {
    std::println("Selected physical device:");
    std::println("\t{}", result.deviceName);
    std::println(""); // Padding
  }
}

VulkanContext::PhysicalDeviceCandidateResult VulkanContext::EvaluatePhysicalDevice(const vk::PhysicalDevice candidate) const {
  PhysicalDeviceCandidateResult result;
  result.deviceName = candidate.getProperties().deviceName.data();
  result.SetScore(0);

  const vk::PhysicalDeviceProperties properties = candidate.getProperties();

  result.deviceInfos.emplace_back(std::format("Driver version: {}", properties.driverVersion));
  result.deviceInfos.emplace_back(std::format("Vendor ID: {}", properties.vendorID));
  result.deviceInfos.emplace_back(std::format("Device type: {}", vk::to_string(properties.deviceType)));

  const auto& limits = properties.limits;

  result.deviceInfos.emplace_back("\n\tImage limits");
  result.deviceInfos.emplace_back(std::format("Max image dimension 1D: {}", limits.maxImageDimension1D));
  result.deviceInfos.emplace_back(std::format("Max image dimension 2D: {}", limits.maxImageDimension2D));
  result.deviceInfos.emplace_back(std::format("Max image dimension 3D: {}", limits.maxImageDimension3D));
  result.deviceInfos.emplace_back(std::format("Max image dimension cube: {}", limits.maxImageDimensionCube));
  result.deviceInfos.emplace_back(std::format("Max image array layers: {}", limits.maxImageArrayLayers));
  result.deviceInfos.emplace_back(std::format("Max texel buffer elements: {}", limits.maxTexelBufferElements));

  result.deviceInfos.emplace_back("\n\tBuffer limits");
  result.deviceInfos.emplace_back(std::format("Max uniform buffer range: {}", limits.maxUniformBufferRange));
  result.deviceInfos.emplace_back(std::format("Max storage buffer range: {}", limits.maxStorageBufferRange));

  result.deviceInfos.emplace_back("\n\tPush constant limits");
  result.deviceInfos.emplace_back(std::format("Max push constants size: {}", limits.maxPushConstantsSize));

  result.deviceInfos.emplace_back("\n\tDescriptor set limits");
  result.deviceInfos.emplace_back(std::format("Max bound descriptor sets: {}", limits.maxBoundDescriptorSets));
  result.deviceInfos.emplace_back(std::format("Max per-stage descriptor samplers: {}", limits.maxPerStageDescriptorSamplers));
  result.deviceInfos.emplace_back(std::format("Max per-stage descriptor uniform buffers: {}", limits.maxPerStageDescriptorUniformBuffers));
  result.deviceInfos.emplace_back(std::format("Max per-stage descriptor storage buffers: {}", limits.maxPerStageDescriptorStorageBuffers));
  result.deviceInfos.emplace_back(std::format("Max per-stage descriptor sampled images: {}", limits.maxPerStageDescriptorSampledImages));
  result.deviceInfos.emplace_back(std::format("Max per-stage descriptor storage images: {}", limits.maxPerStageDescriptorStorageImages));

  result.deviceInfos.emplace_back("\n\tVertex and fragment limits");
  result.deviceInfos.emplace_back(std::format("Max vertex input attributes: {}", limits.maxVertexInputAttributes));
  result.deviceInfos.emplace_back(std::format("Max vertex input bindings: {}", limits.maxVertexInputBindings));
  result.deviceInfos.emplace_back(std::format("Max vertex output components: {}", limits.maxVertexOutputComponents));
  result.deviceInfos.emplace_back(std::format("Max fragment input components: {}", limits.maxFragmentInputComponents));
  result.deviceInfos.emplace_back(std::format("Max fragment output attachments: {}", limits.maxFragmentOutputAttachments));
  result.deviceInfos.emplace_back(std::format("Max color attachments: {}", limits.maxColorAttachments));

  result.deviceInfos.emplace_back("\n\tCompute limits");
  result.deviceInfos.emplace_back(std::format("Max compute shared memory size: {}", limits.maxComputeSharedMemorySize));
  result.deviceInfos.emplace_back(std::format("Max compute work group invocations: {}", limits.maxComputeWorkGroupInvocations));
  result.deviceInfos.emplace_back(std::format("Max compute work group size: [{}, {}, {}]", limits.maxComputeWorkGroupSize[0], limits.maxComputeWorkGroupSize[1], limits.maxComputeWorkGroupSize[2]));
  result.deviceInfos.emplace_back(std::format("Max compute work group count: [{}, {}, {}]", limits.maxComputeWorkGroupCount[0], limits.maxComputeWorkGroupCount[1], limits.maxComputeWorkGroupCount[2]));

  result.deviceInfos.emplace_back(""); // Padding

  switch (properties.deviceType) {
  case vk::PhysicalDeviceType::eDiscreteGpu:
    result.AddToScore(1000);
    break;
  case vk::PhysicalDeviceType::eIntegratedGpu:
    result.AddToScore(500);
    break;
  default:
    result.AddToScore(100);
  }

  if (properties.apiVersion < ApiVersion) {
    result.InvalidateScore("Device does not support Vulkan 1.4");
  }

  const bool supportsGraphicsFamily = std::ranges::any_of(
    candidate.getQueueFamilyProperties(),
    [](const vk::QueueFamilyProperties& familyProperties) -> bool {
      return (familyProperties.queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics;
    }
  );

  if (!supportsGraphicsFamily) {
    result.InvalidateScore("Device is missing a graphics capable queue family");
  }

  const std::vector supportedExtensions = candidate.enumerateDeviceExtensionProperties();

  result.deviceInfos.emplace_back("Required device extension support");

  for (const std::string_view requiredExtension : RequiredDeviceExtensions) {
    const bool supported = std::ranges::any_of(
      supportedExtensions,
      [requiredExtension](const vk::ExtensionProperties& extensionProperties) -> bool {
        return requiredExtension == extensionProperties.extensionName;
      }
    );

    result.deviceInfos.emplace_back(std::format("{}: {}", requiredExtension, supported ? "true" : "false"));

    if (!supported) {
      result.InvalidateScore(std::format("Device is missing support for extension {}",requiredExtension));
    }
  }

  const auto features = candidate.getFeatures2<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan11Features,
    vk::PhysicalDeviceVulkan13Features
  >();

  const auto& vk11 = features.get<vk::PhysicalDeviceVulkan11Features>();
  const auto& vk13 = features.get<vk::PhysicalDeviceVulkan13Features>();

  if (!vk11.shaderDrawParameters) {
    result.InvalidateScore("Missing vk11.shaderDrawParameters");
  }

  if (!vk13.synchronization2) {
    result.InvalidateScore("Missing vk13.synchronization2");
  }

  if (!vk13.dynamicRendering) {
    result.InvalidateScore("Missing vk13.dynamicRendering");
  }

  return result;
}

void VulkanContext::CreateDeviceAndQueue() {
  const std::vector queueFamiliesProperties = physicalDevice.getQueueFamilyProperties();
  std::optional<std::uint32_t> graphicsCapableQueueFamily;

  for (std::size_t i = 0; i < queueFamiliesProperties.size(); ++i) {
    const vk::QueueFamilyProperties properties = queueFamiliesProperties[i];
    const vk::QueueFlags flags = properties.queueFlags;
    const bool supportsGraphics = (flags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics;

    if (supportsGraphics) {
      graphicsCapableQueueFamily = i;
      break;
    }
  }

  if (!graphicsCapableQueueFamily) {
    throw std::runtime_error{ "Failed to find graphics queue family" };
  }

  queueFamilyIndex = graphicsCapableQueueFamily.value();

  constexpr float queuePriority{ 1.0F };
  const vk::DeviceQueueCreateInfo deviceQueueInfo{
    .queueFamilyIndex = queueFamilyIndex,
    .queueCount = 1,
    .pQueuePriorities = &queuePriority
  };

  const vk::DeviceCreateInfo logicalDeviceInfo{
    .pNext = &RequiredDeviceFeatures.get<vk::PhysicalDeviceFeatures2>(),
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &deviceQueueInfo,
    .enabledExtensionCount = static_cast<std::uint32_t>(RequiredDeviceExtensions.size()),
    .ppEnabledExtensionNames = RequiredDeviceExtensions.data()
  };

  device = vk::raii::Device{ physicalDevice, logicalDeviceInfo };
  queue = vk::raii::Queue{ device, queueFamilyIndex, 0 };
}
