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
  static const bool ValidationLayersEnabled;
  static const bool EnableInfoLogs;
  static const std::vector<const char*> RequiredValidationLayers;
  static const std::vector<const char*> RequiredDeviceExtensions;
  vk::PhysicalDeviceFeatures2& GetRequiredDeviceFeatures() noexcept;
  bool IsRequiredDeviceFeaturesSupported(const vk::PhysicalDevice& physicalDevice) const noexcept;

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

  struct PhysicalDeviceCandidateResult {
    std::string deviceName;
    std::vector<std::string> deviceInfos;

    void SetScore(const std::uint32_t newScore) noexcept {
      score = newScore;
    }

    void AddToScore(const std::uint32_t add) noexcept {
      score = add;
    }

    std::uint32_t GetScore() const noexcept {
      return unfit ? 0 : score;
    }

    void InvalidateScore(const std::string& reason) noexcept {
      unfit = true;
      invalidationReasons.emplace_back(reason);
    }

    std::span<const std::string> GetInvalidationReasons() const noexcept {
      return invalidationReasons;
    }

  private:

    std::uint32_t score{ 0 };
    bool unfit{ false };
    std::vector<std::string> invalidationReasons;

  };

  PhysicalDeviceCandidateResult EvaluatePhysicalDevice(vk::PhysicalDevice candidate) const;

  void CreateDeviceAndQueue();

};