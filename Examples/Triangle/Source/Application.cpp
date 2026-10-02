#include <print>
#include <chrono>

#include <SDL3/SDL.h>

#include "Application.hpp"

Application::Application()
  :
  window{ "Triangle Example", 1000, 1000 },
  vulkanMemoryAllocator{ vulkanContext },
  swapchain{ vulkanContext, window },
  frameGraphicsResources{ vulkanContext, vulkanMemoryAllocator }
{}

Application::~Application() noexcept {
  vulkanContext.device.waitIdle();
}

void Application::Run() {
  running = true;

  using Clock = std::chrono::steady_clock;
  using Duration = std::chrono::duration<float>;
  using TimePoint = std::chrono::time_point<Clock>;
  TimePoint oldTime = Clock::now();

  float elapsedTimeSinceLog{ 0 };
  int frameCountSinceLog{ 0 };

  while (running) {
    const TimePoint currentTime = Clock::now();
    const float frameTime = static_cast<Duration>(currentTime - oldTime).count();
    oldTime = currentTime;

    frameCountSinceLog += 1;
    elapsedTimeSinceLog += frameTime;

    if (elapsedTimeSinceLog >= 1.0F) {
      std::println("FPS: {}", frameCountSinceLog);
      frameCountSinceLog = 0;
      elapsedTimeSinceLog = 0;
    }

    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_EVENT_QUIT:
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        running = false;
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        swapchain.isDirty = true;
      default:
        break;
      }
    }

    Render();
  }
}

void Application::Render() {
  if (window.GetState() == Window::State::Minimized) {
    return;
  }

  if (swapchain.isDirty) {
    swapchain.Recreate({
      .preferredMaxFramesInFlight = 2,
      .preferredPresentMode = vk::PresentModeKHR::eMailbox,
      .preferredAlphaComposite = vk::CompositeAlphaFlagBitsKHR::eOpaque,
    });

    frameGraphicsResources.Recreate({
      .maxFramesInFlight = swapchain.maxFramesInFlight,
      .createColorImages = false,
      .createDepthImages = false,
    });
  }

  WaitForFrameFence();

  const auto imageIndexResult = GetNextSwapchainImage();
  if (!imageIndexResult) {
    return;
  }
  const std::uint32_t imageIndex = *imageIndexResult;

  vulkanContext.device.resetFences(*frameGraphicsResources.fencesFrameInFlight[frameGraphicsResources.frameIndex]);

  vk::raii::CommandBuffer& commandBuffer = frameGraphicsResources.commandBuffers[frameGraphicsResources.frameIndex];
  commandBuffer.reset();
  commandBuffer.begin({});

  MakeSwapchainImageDrawOptimal(imageIndex);
  DrawTriangle(imageIndex);
  MakeSwapchainImagePresentOptimal(imageIndex);

  commandBuffer.end();

  SubmitCommandBuffer(imageIndex);
  frameGraphicsResources.IncrementFrameIndex();
}

void Application::WaitForFrameFence() {
  vk::raii::Fence& fenceFrameInFlight = frameGraphicsResources.fencesFrameInFlight[frameGraphicsResources.frameIndex];

  const vk::Result fenceResult = vulkanContext.device.waitForFences(
    *fenceFrameInFlight,
    vk::True,
    std::numeric_limits<std::uint64_t>::max()
  );

  if (fenceResult != vk::Result::eSuccess) {
    throw std::runtime_error{ "Failed to wait for fence frame in flight" };
  }
}

std::optional<std::uint32_t> Application::GetNextSwapchainImage() {
  vk::raii::Semaphore& semaphoreImageAvailable = frameGraphicsResources.semaphoresImageAvailable[frameGraphicsResources.frameIndex];

  const auto [acquireImageResult, imageIndex] = swapchain.swapchain.acquireNextImage(
    std::numeric_limits<std::uint64_t>::max(),
    semaphoreImageAvailable,
    nullptr
  );

  if (acquireImageResult == vk::Result::eErrorOutOfDateKHR) {
    swapchain.isDirty = true;
    return {};
  }

  if (acquireImageResult != vk::Result::eSuccess && acquireImageResult != vk::Result::eSuboptimalKHR) {
    throw std::runtime_error{ "Failed to acquire swapchain image" };
  }

  return imageIndex;
}

void Application::MakeSwapchainImageDrawOptimal(const std::uint32_t imageIndex) {
  vk::raii::CommandBuffer& commandBuffer = frameGraphicsResources.commandBuffers[frameGraphicsResources.frameIndex];

  vk::ImageMemoryBarrier2 swapchainBarrierWrite{
    .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .srcAccessMask = vk::AccessFlagBits2::eNone,
    .dstStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .dstAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
    .oldLayout = vk::ImageLayout::eUndefined,
    .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
    .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
    .image = swapchain.images[imageIndex],
    .subresourceRange = {
      .aspectMask = vk::ImageAspectFlagBits::eColor,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  commandBuffer.pipelineBarrier2({
    .dependencyFlags = {},
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &swapchainBarrierWrite
  });
}

void Application::DrawTriangle(const std::uint32_t imageIndex) {
  vk::raii::CommandBuffer& commandBuffer = frameGraphicsResources.commandBuffers[frameGraphicsResources.frameIndex];

  vk::RenderingAttachmentInfo colorAttachmentInfo{
    .imageView = swapchain.imageViews[imageIndex],
    .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .loadOp = vk::AttachmentLoadOp::eClear,
    .storeOp = vk::AttachmentStoreOp::eStore,
    .clearValue = vk::ClearColorValue{ 0.0F, 0.0F, 0.0F, 1.0F },
  };

  vk::RenderingInfo renderingInfo{
    .renderArea = {
      .offset = { .x = 0, .y = 0 },
      .extent = swapchain.extent,
    },
    .layerCount = 1,
    .colorAttachmentCount = 1,
    .pColorAttachments = &colorAttachmentInfo,
  };

  commandBuffer.beginRendering(renderingInfo);

  commandBuffer.setViewport(
    0,
    vk::Viewport(
      0.0F,
      0.0F,
      static_cast<float>(swapchain.extent.width),
      static_cast<float>(swapchain.extent.height),
      0.0F,
      1.0F
    )
  );

  commandBuffer.setScissor(
    0,
    vk::Rect2D{
      .offset = vk::Offset2D{ .x = 0, .y = 0 },
      .extent = swapchain.extent,
    }
  );

  VulkanPipeline& pipeline = GetTrianglePipeline(swapchain.surfaceFormat.format);

  commandBuffer.bindPipeline(
    vk::PipelineBindPoint::eGraphics,
    pipeline.GetPipeline()
  );

  commandBuffer.draw(3, 1, 0, 0);

  commandBuffer.endRendering();
}

VulkanPipeline& Application::GetTrianglePipeline(const vk::Format format) {
  if (trianglePipelines.contains(format)) {
    return *trianglePipelines.at(format);
  }

  using namespace vulkan_pipeline_options;

  trianglePipelines.try_emplace(
    format,
    std::make_unique<VulkanPipeline>(
      vulkanContext,
      GraphicsPipelineInfo{
        .shaderStages = {
          ShaderStage{
            .stage = vk::ShaderStageFlagBits::eVertex,
            .shaderPath = "Resource/Triangle.spv",
            .entryPoint = "VertexMain",
          },
          ShaderStage{
            .stage = vk::ShaderStageFlagBits::eFragment,
            .shaderPath = "Resource/Triangle.spv",
            .entryPoint = "FragmentMain",
          },
        },
        .vertexInputInfo = {},
        .inputAssemblyInfo = vk::PipelineInputAssemblyStateCreateInfo{
          .topology = vk::PrimitiveTopology::eTriangleList,
        },
        .viewportInfo = {
          .viewports = { vk::Viewport{} },
          .scissors = { vk::Rect2D{} },
        },
        .rasterizationInfo = vk::PipelineRasterizationStateCreateInfo{
          .depthClampEnable = vk::False,
          .rasterizerDiscardEnable = vk::False,
          .polygonMode = vk::PolygonMode::eFill,
          .cullMode = vk::CullModeFlagBits::eNone,
          .frontFace = vk::FrontFace::eClockwise,
          .depthBiasEnable = vk::False,
          .lineWidth = 1.0F,
        },
        .multisamplingInfo = vk::PipelineMultisampleStateCreateInfo{
          .rasterizationSamples = vk::SampleCountFlagBits::e1,
          .sampleShadingEnable = vk::False,
        },
        .depthStencilInfo = vk::PipelineDepthStencilStateCreateInfo{
          .depthTestEnable = vk::False,
          .depthWriteEnable = vk::False,
          .depthBoundsTestEnable = vk::False,
          .stencilTestEnable = vk::False,
        },
        .colorBlendInfo = ColorBlendInfo{
          .logicOpEnable = vk::False,
          .attachments = {
            vk::PipelineColorBlendAttachmentState{
              .blendEnable = vk::False,
              .colorWriteMask = vk::ColorComponentFlagBits::eR |
                                vk::ColorComponentFlagBits::eG |
                                vk::ColorComponentFlagBits::eB |
                                vk::ColorComponentFlagBits::eA
            },
          },
        },
        .dynamicStates = {
          vk::DynamicState::eViewport,
          vk::DynamicState::eScissor,
        },
        .descriptorSets = {},
        .pushConstants = {},
      },
      RenderingInfo{
        .colorAttachmentFormats = { swapchain.surfaceFormat.format },
      }
    )
  );

  return *trianglePipelines.at(format);
}

void Application::MakeSwapchainImagePresentOptimal(const std::uint32_t imageIndex) {
  vk::raii::CommandBuffer& commandBuffer = frameGraphicsResources.commandBuffers[frameGraphicsResources.frameIndex];

  vk::ImageMemoryBarrier2 swapchainBarrierPresent{
    .srcStageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .srcAccessMask = vk::AccessFlagBits2::eColorAttachmentWrite,
    .dstStageMask = vk::PipelineStageFlagBits2::eBottomOfPipe,
    .dstAccessMask = {},
    .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .newLayout = vk::ImageLayout::ePresentSrcKHR,
    .srcQueueFamilyIndex = vk::QueueFamilyIgnored,
    .dstQueueFamilyIndex = vk::QueueFamilyIgnored,
    .image = swapchain.images[imageIndex],
    .subresourceRange = {
      .aspectMask = vk::ImageAspectFlagBits::eColor,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
  };

  commandBuffer.pipelineBarrier2({
    .dependencyFlags = {},
    .imageMemoryBarrierCount = 1,
    .pImageMemoryBarriers = &swapchainBarrierPresent
  });
}

void Application::SubmitCommandBuffer(const std::uint32_t imageIndex) {
  const std::uint32_t frameIndex = frameGraphicsResources.frameIndex;
  vk::raii::CommandBuffer& commandBuffer = frameGraphicsResources.commandBuffers[frameIndex];
  vk::raii::Fence& fenceFrameInFlight = frameGraphicsResources.fencesFrameInFlight[frameIndex];
  vk::raii::Semaphore& semaphoreRenderFinished = frameGraphicsResources.semaphoresRenderFinished[imageIndex];
  vk::raii::Semaphore& semaphoreImageAvailable = frameGraphicsResources.semaphoresImageAvailable[frameIndex];
  vk::PipelineStageFlags waitDestinationStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

  const vk::SubmitInfo submitInfo{
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &*semaphoreImageAvailable,
    .pWaitDstStageMask = &waitDestinationStageMask,
    .commandBufferCount = 1,
    .pCommandBuffers = &*commandBuffer,
    .signalSemaphoreCount = 1,
    .pSignalSemaphores = &*semaphoreRenderFinished
  };

  vulkanContext.queue.submit(submitInfo, *fenceFrameInFlight);

  const vk::PresentInfoKHR presentInfo{
    .waitSemaphoreCount = 1,
    .pWaitSemaphores = &*semaphoreRenderFinished,
    .swapchainCount = 1,
    .pSwapchains = &*swapchain.swapchain,
    .pImageIndices = &imageIndex
  };

  const vk::Result presentResult = vulkanContext.queue.presentKHR(presentInfo);

  if (presentResult == vk::Result::eSuboptimalKHR ||
    presentResult == vk::Result::eErrorOutOfDateKHR) {
    swapchain.isDirty = true;
  }
}
