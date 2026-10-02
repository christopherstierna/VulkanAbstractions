#pragma once

#include <vector>
#include <unordered_map>

#include <vulkan/vulkan_raii.hpp>

#include "VulkanPipelineOptions.hpp"

class VulkanContext;

class VulkanPipeline {
public:

  VulkanPipeline(
    VulkanContext& vulkanContext,
    const vulkan_pipeline_options::GraphicsPipelineInfo& pipelineInfo,
    const vulkan_pipeline_options::RenderingInfo& renderingInfo
  );

  VulkanPipeline(
    VulkanContext& vulkanContext,
    const vulkan_pipeline_options::ComputePipelineInfo& pipelineInfo
  );

  ~VulkanPipeline() noexcept = default;

  VulkanPipeline(const VulkanPipeline&) = delete;
  VulkanPipeline& operator=(const VulkanPipeline&) noexcept = delete;

  VulkanPipeline(VulkanPipeline&&) noexcept = delete;
  VulkanPipeline& operator=(VulkanPipeline&& otherPipeline) noexcept = delete;

private:

  VulkanContext& vulkanContext;

  std::vector<vk::raii::DescriptorSetLayout> descriptorSetLayouts;
  vk::raii::PipelineLayout pipelineLayout{ nullptr };
  vk::raii::Pipeline pipeline{ nullptr };
  vk::raii::DescriptorPool descriptorPool{ nullptr };
  vk::raii::DescriptorSets descriptorSets{ nullptr };
  std::vector<std::uint32_t> descriptorSetOffsets;

  std::unordered_map<vk::DescriptorType, std::uint32_t> descriptorTypeCounts;
  std::vector<vk::DescriptorSetLayout> descriptorSetLayoutHandles;

  void CreatePipelineLayout(
    const std::vector<vulkan_pipeline_options::DescriptorSet>& descriptorSetsInfo,
    const std::vector<vk::PushConstantRange>& pushConstants
  );

  void AllocateDescriptorSets();

public:

  vk::Pipeline GetPipeline() noexcept;
  vk::PipelineLayout GetLayout() noexcept;
  vk::DescriptorSet GetDescriptorSet(std::uint32_t setIndex, std::uint32_t copyIndex = 0) noexcept;

  void WriteDescriptors(const std::vector<vulkan_pipeline_options::DescriptorWrite>& descriptorWrites);

};
