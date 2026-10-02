#include <format>
#include <vector>
#include <unordered_map>
#include <filesystem>

#include "VulkanPipeline.hpp"
#include "VulkanContext.hpp"
#include "ReadFile.hpp"

VulkanPipeline::VulkanPipeline(
  VulkanContext& vulkanContext,
  const vulkan_pipeline_options::GraphicsPipelineInfo& pipelineInfo,
  const vulkan_pipeline_options::RenderingInfo& renderingInfo
)
  :
  vulkanContext{ vulkanContext } {

  CreatePipelineLayout(pipelineInfo.descriptorSets, pipelineInfo.pushConstants);

  std::unordered_map<std::filesystem::path, vk::raii::ShaderModule> shaderModules;
  std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
  std::vector<vk::SpecializationInfo> specializationInfos;

  shaderStages.reserve(pipelineInfo.shaderStages.size());
  specializationInfos.reserve(pipelineInfo.shaderStages.size());

  for (const vulkan_pipeline_options::ShaderStage& stage : pipelineInfo.shaderStages) {
    if (!shaderModules.contains(stage.shaderPath)) {
      const auto shaderCodeResult = ReadFileShader(stage.shaderPath);
      if (!shaderCodeResult) {
        throw std::runtime_error{ std::format("Failed to create VulkanPipeline because of error: {}", shaderCodeResult.error()) };
      }

      const std::vector shaderCode = *shaderCodeResult;

      shaderModules.try_emplace(
        stage.shaderPath,
        vulkanContext.device,
        vk::ShaderModuleCreateInfo{
          .codeSize = shaderCode.size() * sizeof(std::uint32_t),
          .pCode = shaderCode.data(),
        }
      );
    }

    specializationInfos.emplace_back(vk::SpecializationInfo{
      .mapEntryCount = static_cast<std::uint32_t>(stage.specializationInfo.entries.size()),
      .pMapEntries = stage.specializationInfo.entries.data(),
      .dataSize = stage.specializationInfo.dataSize,
      .pData = stage.specializationInfo.data,
    });

    shaderStages.emplace_back(vk::PipelineShaderStageCreateInfo{
      .stage = stage.stage,
      .module = shaderModules.at(stage.shaderPath),
      .pName = stage.entryPoint.c_str(),
      .pSpecializationInfo = &specializationInfos.back(),
    });
  }

  const vk::PipelineVertexInputStateCreateInfo vertexInputInfo{
    .vertexBindingDescriptionCount = static_cast<std::uint32_t>(
      pipelineInfo.vertexInputInfo.vertexAttributeBindings.size()
    ),
    .pVertexBindingDescriptions = pipelineInfo.vertexInputInfo.vertexAttributeBindings.data(),
    .vertexAttributeDescriptionCount = static_cast<std::uint32_t>(
      pipelineInfo.vertexInputInfo.vertexAttributes.size()
    ),
    .pVertexAttributeDescriptions = pipelineInfo.vertexInputInfo.vertexAttributes.data()
  };

  const vk::PipelineViewportStateCreateInfo viewportInfo{
    .viewportCount = static_cast<std::uint32_t>(pipelineInfo.viewportInfo.viewports.size()),
    .pViewports = pipelineInfo.viewportInfo.viewports.data(),
    .scissorCount = static_cast<std::uint32_t>(pipelineInfo.viewportInfo.scissors.size()),
    .pScissors = pipelineInfo.viewportInfo.scissors.data()
  };

  const vk::PipelineColorBlendStateCreateInfo colorBlendInfo{
    .logicOpEnable = pipelineInfo.colorBlendInfo.logicOpEnable,
    .logicOp = pipelineInfo.colorBlendInfo.logicOp,
    .attachmentCount = static_cast<std::uint32_t>(pipelineInfo.colorBlendInfo.attachments.size()),
    .pAttachments = pipelineInfo.colorBlendInfo.attachments.data(),
    .blendConstants = pipelineInfo.colorBlendInfo.blendConstants,
  };

  const vk::PipelineDynamicStateCreateInfo dynamicState{
    .dynamicStateCount = static_cast<std::uint32_t>(pipelineInfo.dynamicStates.size()),
    .pDynamicStates = pipelineInfo.dynamicStates.data()
  };

  const vk::StructureChain pipelineStructureChain{
    vk::GraphicsPipelineCreateInfo{
      .stageCount = static_cast<std::uint32_t>(shaderStages.size()),
      .pStages = shaderStages.data(),
      .pVertexInputState = &vertexInputInfo,
      .pInputAssemblyState = &pipelineInfo.inputAssemblyInfo,
      .pViewportState = &viewportInfo,
      .pRasterizationState = &pipelineInfo.rasterizationInfo,
      .pMultisampleState = &pipelineInfo.multisamplingInfo,
      .pDepthStencilState = &pipelineInfo.depthStencilInfo,
      .pColorBlendState = &colorBlendInfo,
      .pDynamicState = &dynamicState,
      .layout = pipelineLayout,
      .renderPass = nullptr,
    },
    vk::PipelineRenderingCreateInfo{
      .viewMask = renderingInfo.viewMask,
      .colorAttachmentCount = static_cast<std::uint32_t>(renderingInfo.colorAttachmentFormats.size()),
      .pColorAttachmentFormats = renderingInfo.colorAttachmentFormats.data(),
      .depthAttachmentFormat = renderingInfo.depthAttachmentFormat,
      .stencilAttachmentFormat = renderingInfo.stencilAttachmentFormat,
    }
  };

  pipeline = vk::raii::Pipeline{
    vulkanContext.device,
    nullptr,
    pipelineStructureChain.get<vk::GraphicsPipelineCreateInfo>()
  };

  AllocateDescriptorSets();
}

VulkanPipeline::VulkanPipeline(
  VulkanContext& vulkanContext,
  const vulkan_pipeline_options::ComputePipelineInfo& pipelineInfo
)
  :
  vulkanContext{ vulkanContext } {

  CreatePipelineLayout(pipelineInfo.descriptorSets, pipelineInfo.pushConstants);

  const auto shaderCodeResult = ReadFileShader(pipelineInfo.shaderStage.shaderPath);
  if (!shaderCodeResult) {
    throw std::runtime_error{ std::format("Failed to create VulkanPipeline because of error: {}", shaderCodeResult.error()) };
  }

  const auto shaderCode = *shaderCodeResult;

  const vk::ShaderModuleCreateInfo shaderModuleCreateInfo{
    .codeSize = shaderCode.size() * sizeof(std::uint32_t),
    .pCode = shaderCode.data()
  };

  vk::raii::ShaderModule shaderModule = vk::raii::ShaderModule{
    vulkanContext.device,
    shaderModuleCreateInfo
  };

  const vk::SpecializationInfo specializationInfo{
    .mapEntryCount = static_cast<std::uint32_t>(pipelineInfo.shaderStage.specializationInfo.entries.size()),
    .pMapEntries = pipelineInfo.shaderStage.specializationInfo.entries.data(),
    .dataSize = pipelineInfo.shaderStage.specializationInfo.dataSize,
    .pData = pipelineInfo.shaderStage.specializationInfo.data,
  };

  const vk::PipelineShaderStageCreateInfo computeStageCreateInfo{
    .flags = {},
    .stage = pipelineInfo.shaderStage.stage,
    .module = shaderModule,
    .pName = pipelineInfo.shaderStage.entryPoint.c_str(),
    .pSpecializationInfo = &specializationInfo
  };

  const vk::ComputePipelineCreateInfo pipelineCreateInfo{
    .flags = {},
    .stage = computeStageCreateInfo,
    .layout = pipelineLayout
  };

  pipeline = vk::raii::Pipeline(
    vulkanContext.device,
    nullptr,
    pipelineCreateInfo
  );

  AllocateDescriptorSets();
}

void VulkanPipeline::CreatePipelineLayout(
  const std::vector<vulkan_pipeline_options::DescriptorSet>& descriptorSetsInfo,
  const std::vector<vk::PushConstantRange>& pushConstants
) {
  // First set has offset 0
  descriptorSetOffsets.emplace_back(0);

  for (const auto& descriptorSet : descriptorSetsInfo) {

    for (const auto& binding : descriptorSet.bindings) {
      const std::uint32_t count = binding.descriptorCount * descriptorSet.numCopies;
      if (descriptorTypeCounts.contains(binding.descriptorType)) {
        descriptorTypeCounts.at(binding.descriptorType) += count;
      } else {
        descriptorTypeCounts.try_emplace(binding.descriptorType, count);
      }
    }

    for (std::uint32_t i = 0; i < descriptorSet.numCopies; ++i) {
      descriptorSetLayouts.emplace_back(
        vulkanContext.device,
        vk::DescriptorSetLayoutCreateInfo{
          .bindingCount = static_cast<std::uint32_t>(descriptorSet.bindings.size()),
          .pBindings = descriptorSet.bindings.data()
        }
      );

      descriptorSetLayoutHandles.emplace_back(descriptorSetLayouts.back());
    }

    descriptorSetOffsets.emplace_back(descriptorSet.numCopies);
  }

  pipelineLayout = vk::raii::PipelineLayout{
    vulkanContext.device,
    vk::PipelineLayoutCreateInfo{
      .setLayoutCount = static_cast<std::uint32_t>(descriptorSetLayoutHandles.size()),
      .pSetLayouts = descriptorSetLayoutHandles.data(),
      .pushConstantRangeCount = static_cast<std::uint32_t>(pushConstants.size()),
      .pPushConstantRanges = pushConstants.data()
    }
  };
}

void VulkanPipeline::AllocateDescriptorSets() {
  if (descriptorSetLayoutHandles.size() == 0 || descriptorTypeCounts.size() == 0) {
    return;
  }

  std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
  for (const auto& [type, count] : descriptorTypeCounts) {
    descriptorPoolSizes.emplace_back(type, count);
  }

  descriptorPool = vk::raii::DescriptorPool{
    vulkanContext.device,
    vk::DescriptorPoolCreateInfo{
      .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
      .maxSets = static_cast<std::uint32_t>(descriptorSetLayoutHandles.size()),
      .poolSizeCount = static_cast<std::uint32_t>(descriptorPoolSizes.size()),
      .pPoolSizes = descriptorPoolSizes.data()
    }
  };

  const vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo{
    .descriptorPool = descriptorPool,
    .descriptorSetCount = static_cast<std::uint32_t>(descriptorSetLayouts.size()),
    .pSetLayouts = descriptorSetLayoutHandles.data(),
  };

  descriptorSets = vulkanContext.device.allocateDescriptorSets(descriptorSetAllocateInfo);
}

vk::Pipeline VulkanPipeline::GetPipeline() noexcept {
  return pipeline;
}

vk::PipelineLayout VulkanPipeline::GetLayout() noexcept {
  return pipelineLayout;
}

vk::DescriptorSet VulkanPipeline::GetDescriptorSet(const std::uint32_t setIndex, const std::uint32_t copyIndex) noexcept {
  return descriptorSets[descriptorSetOffsets[setIndex] + copyIndex];
}

void VulkanPipeline::WriteDescriptors(const std::vector<vulkan_pipeline_options::DescriptorWrite>& descriptorWrites) {
  using namespace vulkan_pipeline_options;

  std::vector<vk::WriteDescriptorSet> writes;
  writes.reserve(descriptorWrites.size());

  for (const auto& descriptorWrite : descriptorWrites) {
    vk::WriteDescriptorSet write;
    write.dstSet = GetDescriptorSet(descriptorWrite.set, descriptorWrite.copyIndex);
    write.dstBinding = descriptorWrite.binding;
    write.dstArrayElement = descriptorWrite.arrayElement;
    write.descriptorType = descriptorWrite.descriptorType;

    write.descriptorCount = 0;
    write.pBufferInfo = nullptr;
    write.pImageInfo = nullptr;

    using BufferWrites = std::vector<vk::DescriptorBufferInfo>;
    using ImageWrites = std::vector<vk::DescriptorImageInfo>;

    const bool isABufferWrite = std::holds_alternative<BufferWrites>(descriptorWrite.writes);

    if (isABufferWrite) {
      const auto& bufferWrites = std::get<BufferWrites>(descriptorWrite.writes);

      write.descriptorCount = bufferWrites.size();
      write.pBufferInfo = bufferWrites.data();
    } else {
      const auto& imageWrites = std::get<ImageWrites>(descriptorWrite.writes);

      write.descriptorCount = imageWrites.size();
      write.pImageInfo = imageWrites.data();
    }

    writes.emplace_back(write);
  }

  vulkanContext.device.updateDescriptorSets(writes, {});
}
