#include "renderer.hpp"
#include "mesh.hpp"
#include "swapchain.hpp"
#include "window.hpp"
#include <array>
#include <cassert>
#include <glm/ext/scalar_constants.hpp>
#include <vulkan/vulkan_core.h>

using namespace std;

namespace Magma {

Renderer::Renderer(Device &device, Window &window,
                   VkDescriptorSetLayout descriptorSetLayout)
    : device{device}, window{window} {
  swapChain = make_unique<SwapChain>(device, window.getExtent());
  createPipelineLayout(descriptorSetLayout);
  createPipeline(swapChain->getRenderPass());
  createCommandBuffers();
}

Renderer::~Renderer() {
  vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
  vkFreeCommandBuffers(device.device(), device.getCommandPool(), 1,
                       &imageAcquireCommandBuffer);
}

void Renderer::recreateSwapChain() {
  auto extent = window.getExtent();
  while (extent.width == 0 || extent.height == 0) {
    extent = window.getExtent();
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device.device());

  shared_ptr<SwapChain> oldSwapChain = std::move(swapChain);
  swapChain = make_unique<SwapChain>(device, extent, oldSwapChain);

  if (!oldSwapChain->compareSwapFormats(*swapChain.get()))
    throw runtime_error("Swap chain image(or depth) format has changed!");
}

void Renderer::createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout) {

  vector<VkDescriptorSetLayout> layouts{descriptorSetLayout};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();

  if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS)
    throw runtime_error("Failed to create pipeline layout!");
}

void Renderer::createPipeline(VkRenderPass renderPass) {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before pipeline layout!");

  PipelineConfigInfo piplineConfigInfo = {};
  Pipeline::defaultPipelineConfig(piplineConfigInfo);
  piplineConfigInfo.renderPass = renderPass;
  piplineConfigInfo.pipelineLayout = pipelineLayout;

  pipeline =
      make_unique<Pipeline>(device, "src/shaders/shader.vert.spv",
                            "src/shaders/shader.frag.spv", piplineConfigInfo);
}

void Renderer::createCommandBuffers() {
  commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = device.getCommandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(device.device(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS)
    throw runtime_error("Failed to allocate command buffers!");
}

VkCommandBuffer Renderer::beginFrame() {
  auto result = swapChain->acquireNextImage(&currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return nullptr;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    throw runtime_error("Failed to acquire next swap chain image!");

  VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    throw runtime_error("Failed to begin recording command buffer!");

  return commandBuffer;
}

void Renderer::recordCommandBuffer(VkCommandBuffer commandBuffer) {
  VkRenderPassBeginInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = swapChain->getRenderPass();
  renderPassInfo.framebuffer = swapChain->getFrameBuffer(currentImageIndex);
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChain->extent();

  array<VkClearValue, 2> clearValues = {};
  clearValues[0].color = {{0.53f, 0.81f, 0.92f, 1.0f}};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = static_cast<float>(swapChain->extent().height);
  viewport.width = static_cast<float>(swapChain->extent().width);
  viewport.height = -static_cast<float>(swapChain->extent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = swapChain->extent();
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  pipeline->bind(commandBuffer);
}

void Renderer::endRenderPass(VkCommandBuffer commandBuffer) {
  vkCmdEndRenderPass(commandBuffer);
}

void Renderer::endFrame() {
  VkCommandBuffer commandBuffer = getCurrentCommandBuffer();
  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    throw runtime_error("Failed to record command buffer!");

  auto result =
      swapChain->submitCommandBuffer(&commandBuffer, &currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      window.wasWindowResized()) {
    window.resetWindowResizedFlag();
    recreateSwapChain();
    currentFrameIndex = 0;
    return;
  } else if (result != VK_SUCCESS)
    throw runtime_error("Failed to present swap chain image!");

  currentFrameIndex = (currentFrameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
}
} // namespace Magma
