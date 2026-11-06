#include <cstdint>
#include <cmath>
#include <print>

#include <vulkan/vulkan_raii.hpp>
#include <VulkanAbstractions/VulkanContext.hpp>
#include <VulkanAbstractions/VulkanMemoryAllocator.hpp>
#include <VulkanAbstractions/VulkanPipeline.hpp>
#include <VulkanAbstractions/VulkanBuffer.hpp>

constexpr unsigned int threadCount{ 10000 };
constexpr unsigned int samplesPerThread{ 100000 };

namespace shader_struct {

struct PushConstant {
  std::uint32_t threadCount;
  std::uint32_t sampleCount;
};

}

void EstimatePi() {
  VulkanContext vulkanContext;
  VulkanMemoryAllocator vulkanMemoryAllocator{ vulkanContext };
  
  VulkanBuffer threadPiBuffer = vulkanMemoryAllocator.AllocateBuffer({
    .size = sizeof(std::uint32_t) * threadCount,
    .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
    .memoryAccessMode = VulkanMemoryAccessMode::DeviceLocal,
  });

  VulkanBuffer hostPiBuffer = vulkanMemoryAllocator.AllocateBuffer({
    .size = threadPiBuffer.size,
    .usage = vk::BufferUsageFlagBits::eTransferDst,
    .memoryAccessMode = VulkanMemoryAccessMode::HostRandomReadAndWrite,
  });

  VulkanPipeline estimatePiPipeline{
    vulkanContext,
    vulkan_pipeline_options::ComputePipelineInfo{
      .shaderStage = {
        .stage = vk::ShaderStageFlagBits::eCompute,
        .shaderPath = "Resource/Spv/EstimatePi.spv",
        .entryPoint = "ComputeMain",
      },
      .descriptorSets = {
        vulkan_pipeline_options::DescriptorSet{
          .numCopies = 1,
          .bindings = { 
            vk::DescriptorSetLayoutBinding{
              .binding = 0,
              .descriptorType = vk::DescriptorType::eStorageBuffer,
              .descriptorCount = 1,
              .stageFlags = vk::ShaderStageFlagBits::eCompute,
            }
          },
        }
      },
      .pushConstants = {
        vk::PushConstantRange{
          .stageFlags = vk::ShaderStageFlagBits::eCompute,
          .offset = 0,
          .size = sizeof(shader_struct::PushConstant),
        },
      },
    }
  };

  const vk::CommandPoolCreateInfo commandPoolInfo{
    .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
    .queueFamilyIndex = vulkanContext.queueFamilyIndex
  };

  vk::raii::CommandPool commandPool = vk::raii::CommandPool{ vulkanContext.device, commandPoolInfo };

  const vk::CommandBufferAllocateInfo allocateInfo{
    .commandPool = commandPool,
    .level = vk::CommandBufferLevel::ePrimary,
    .commandBufferCount = 1,
  };

  vk::raii::CommandBuffer commandBuffer = std::move(vk::raii::CommandBuffers{ vulkanContext.device, allocateInfo }.front());

  commandBuffer.reset();
  commandBuffer.begin({});

  estimatePiPipeline.WriteDescriptors({
    vulkan_pipeline_options::DescriptorWrite{
      .set = 0,
      .binding = 0,
      .arrayElement = 0,
      .descriptorType = vk::DescriptorType::eStorageBuffer,
      .writes = std::vector<vk::DescriptorBufferInfo>{
        vk::DescriptorBufferInfo{
          .buffer = threadPiBuffer.buffer,
          .offset = 0,
          .range = threadPiBuffer.size,
        },
      },
    },
  });

  commandBuffer.bindPipeline(
    vk::PipelineBindPoint::eCompute,
    estimatePiPipeline.GetPipeline()
  );

  commandBuffer.bindDescriptorSets(
    vk::PipelineBindPoint::eCompute,
    estimatePiPipeline.GetLayout(),
    0,
    estimatePiPipeline.GetDescriptorSet(0),
    nullptr
  );

  shader_struct::PushConstant pushConstant;
  pushConstant.threadCount = threadCount;
  pushConstant.sampleCount = samplesPerThread;

  commandBuffer.pushConstants(
    estimatePiPipeline.GetLayout(),
    vk::ShaderStageFlagBits::eCompute,
    0,
    sizeof(pushConstant),
    &pushConstant
  );

  commandBuffer.dispatch(
    static_cast<std::uint32_t>(std::ceil(threadCount / 256.0F)),
    1,
    1
  );

  commandBuffer.end();

  vulkanContext.queue.submit(
    vk::SubmitInfo{
      .commandBufferCount = 1,
      .pCommandBuffers = &*commandBuffer
    },
    nullptr
  );

  vulkanContext.device.waitIdle();

  commandBuffer.reset();
  commandBuffer.begin({});

  commandBuffer.copyBuffer(
    threadPiBuffer.buffer,
    hostPiBuffer.buffer,
    vk::BufferCopy{
      .srcOffset = 0,
      .dstOffset = 0,
      .size = threadPiBuffer.size,
    }
  );

  commandBuffer.end();

  vulkanContext.queue.submit(
    vk::SubmitInfo{
      .commandBufferCount = 1,
      .pCommandBuffers = &*commandBuffer
    },
    nullptr
  );

  vulkanContext.device.waitIdle();

  float pi = 0;

  for (int i = 0; i < threadCount; ++i) {
    const float threadEstimation = *(static_cast<float*>(hostPiBuffer.mappedData) + i);
    pi += threadEstimation / threadCount;
  }

  std::println("Pi is approximately {}", pi);
}