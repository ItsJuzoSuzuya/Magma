#include "magma_renderer.hpp"
#include "magma_device.hpp"
#include "magma_swap_chain.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace magma {

MagmaRenderer::MagmaRenderer(MagmaWindow &window, MagmaDevice &device)
    : magmaWindow{window}, magmaDevice{device} {
  recreateSwapChain();
  createCommandBuffers();
}

MagmaRenderer::~MagmaRenderer() { freeCommandBuffers(); }

void MagmaRenderer::recreateSwapChain() {
  auto extent = magmaWindow.getExtent();
  while (extent.width == 0 || extent.height == 0) {
    extent = magmaWindow.getExtent();
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(magmaDevice.device());

  if (magmaSwapChain == nullptr) {
    magmaSwapChain = std::make_unique<MagmaSwapChain>(magmaDevice, extent);
  } else {
    std::shared_ptr<MagmaSwapChain> oldSwapChain = std::move(magmaSwapChain);
    magmaSwapChain =
        std::make_unique<MagmaSwapChain>(magmaDevice, extent, oldSwapChain);

    if (!oldSwapChain->compareSwapFormats(*magmaSwapChain.get())) {
      throw std::runtime_error(
          "Swap chain image(or depth) format has changed!");
    }
  }
}

void MagmaRenderer::createCommandBuffers() {
  commandBuffers.resize(MagmaSwapChain::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = magmaDevice.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(magmaDevice.device(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command buffer!");
  }
}
void MagmaRenderer::freeCommandBuffers() {
  vkFreeCommandBuffers(magmaDevice.device(), magmaDevice.getCommandPool(),
                       static_cast<uint32_t>(commandBuffers.size()),
                       commandBuffers.data());
  commandBuffers.clear();
}

VkCommandBuffer MagmaRenderer::beginFrame() {
  assert(!isFrameStarted && "Can't call beginFrame while already in progress");

  auto result = magmaSwapChain->acquireNextImage(&currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    recreateSwapChain();
    return nullptr;
  }

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  isFrameStarted = true;

  auto commandBuffer = getCurrentCommandBuffer();

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
    throw std::runtime_error("failed to begin recording command buffer!");
  }

  return commandBuffer;
}

void MagmaRenderer::endFrame() {
  assert(isFrameStarted &&
         "Can't call endFrame while frame is not in progress!");
  auto commandBuffer = getCurrentCommandBuffer();

  if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
    throw std::runtime_error("failed to record command buffer!");
  }
  auto result =
      magmaSwapChain->submitCommandBuffers(&commandBuffer, &currentImageIndex);
  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      magmaWindow.wasWindowResized()) {
    magmaWindow.resetWindowResizedFlag();
    recreateSwapChain();
  } else if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image");
  }

  isFrameStarted = false;
  currentFrameIndex =
      (currentFrameIndex + 1) % MagmaSwapChain::MAX_FRAMES_IN_FLIGHT;
}

void MagmaRenderer::beginSwapChainRenderPass(VkCommandBuffer commandBuffer) {
  assert(isFrameStarted &&
         "Can't call beginSwapChainRenderPass while frame is not in progress!");
  assert(commandBuffer == getCurrentCommandBuffer() &&
         "Cant begin render pass on command buffer from a different frame!");

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = magmaSwapChain->getRenderPass();
  renderPassInfo.framebuffer =
      magmaSwapChain->getFrameBuffer(currentImageIndex);

  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = magmaSwapChain->getSwapChainExtent();

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {0.01f, 0.01f, 0.01f, 1.0f};
  clearValues[1].depthStencil = {1.0f, 0};
  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width =
      static_cast<float>(magmaSwapChain->getSwapChainExtent().width);
  viewport.height =
      static_cast<float>(magmaSwapChain->getSwapChainExtent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  VkRect2D scissor{{0, 0}, magmaSwapChain->getSwapChainExtent()};
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void MagmaRenderer::endSwapChainRenderPass(VkCommandBuffer commandBuffer) {
  assert(isFrameStarted &&
         "Can't call endSwapChainRenderPass while frame is not in progress!");
  assert(commandBuffer == getCurrentCommandBuffer() &&
         "Cant end render pass on command buffer from a different frame!");

  vkCmdEndRenderPass(commandBuffer);
}

} // namespace magma
