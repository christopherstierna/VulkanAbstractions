#pragma once

#include <filesystem>
#include <vector>
#include <array>
#include <string>
#include <variant>

#include <vulkan/vulkan.hpp>

namespace vulkan_pipeline_options {

struct SpecializationInfo {
  std::vector<vk::SpecializationMapEntry> entries;
  std::size_t dataSize;
  void* data;
};

struct ShaderStage {
  vk::ShaderStageFlagBits stage;
  std::filesystem::path shaderPath;
  std::string entryPoint;
  SpecializationInfo specializationInfo;
};

struct VertexInputInfo {
  std::vector<vk::VertexInputBindingDescription> vertexAttributeBindings;
  std::vector<vk::VertexInputAttributeDescription> vertexAttributes;
};

struct ViewportInfo {
  std::vector<vk::Viewport> viewports;
  std::vector<vk::Rect2D> scissors;
};

struct ColorBlendInfo {
  vk::Bool32 logicOpEnable;
  vk::LogicOp logicOp;
  std::vector<vk::PipelineColorBlendAttachmentState> attachments;
  std::array<float, 4> blendConstants;
};

struct DescriptorSet {
  std::uint32_t numCopies{ 1 };
  std::vector<vk::DescriptorSetLayoutBinding> bindings;
};

struct GraphicsPipelineInfo {
  std::vector<ShaderStage> shaderStages;
  VertexInputInfo vertexInputInfo;
  vk::PipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
  ViewportInfo viewportInfo;
  vk::PipelineRasterizationStateCreateInfo rasterizationInfo;
  vk::PipelineMultisampleStateCreateInfo multisamplingInfo;
  vk::PipelineDepthStencilStateCreateInfo depthStencilInfo;
  ColorBlendInfo colorBlendInfo;
  std::vector<vk::DynamicState> dynamicStates;
  std::vector<DescriptorSet> descriptorSets;
  std::vector<vk::PushConstantRange> pushConstants;
};

struct RenderingInfo {
  std::uint32_t viewMask;
  std::vector<vk::Format> colorAttachmentFormats;
  vk::Format depthAttachmentFormat;
  vk::Format stencilAttachmentFormat;
};

struct ComputePipelineInfo {
  ShaderStage shaderStage;
  std::vector<DescriptorSet> descriptorSets;
  std::vector<vk::PushConstantRange> pushConstants;
};

struct DescriptorWrite {
  std::uint32_t copyIndex{ 0 };
  std::uint32_t set;
  std::uint32_t binding;
  std::uint32_t arrayElement;
  vk::DescriptorType descriptorType;
  std::variant<std::vector<vk::DescriptorBufferInfo>, std::vector<vk::DescriptorImageInfo>> writes;
};

}