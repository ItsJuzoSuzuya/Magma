#include "magma.hpp"
#include "magma_pipeline.hpp"
#include "magma_swap_chain.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <glm/fwd.hpp>
#include <iostream>
#include <memory>
#include <numeric>
#include <ostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace magma {

Magma::Magma() {
  loadModels();
  createPipelineLayout();
  createPipeline();
  createCommandBuffers();
}

Magma::~Magma() {
  vkDestroyPipelineLayout(magmaDevice.device(), pipelineLayout, nullptr);
}

void Magma::run() {
  while (!magmaWindow.shouldClose()) {
    glfwPollEvents();
    drawFrame();
  }

  vkDeviceWaitIdle(magmaDevice.device());
}

void Magma::loadModels() {
  std::vector<MagmaModel::Vertex> vertices;
  std::vector<MagmaModel::Vertex> intialVertices{
      {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
  };

  calculateSiepinskiTriangle(intialVertices, &vertices, 0);
  magmaModel = std::make_unique<MagmaModel>(magmaDevice, vertices);
}

void Magma::calculateSiepinskiTriangle(
    std::vector<MagmaModel::Vertex> preVertices,
    std::vector<MagmaModel::Vertex> *result, int counter) {
  if (counter > 4) {
    result->insert(result->end(), preVertices.begin(), preVertices.end());
    return;
  }

  glm::vec2 leftCenter = preVertices[1].position - preVertices[0].position;
  glm::vec2 rightCenter = preVertices[1].position - preVertices[2].position;
  glm::vec2 bottomCenter = preVertices[2].position - preVertices[0].position;

  leftCenter = preVertices[1].position -
               glm::vec2(leftCenter.x / 2.0f, leftCenter.y / 2.0f);
  rightCenter = preVertices[1].position -
                glm::vec2(rightCenter.x / 2.0f, rightCenter.y / 2.0f);
  bottomCenter = preVertices[2].position -
                 glm::vec2(bottomCenter.x / 2.0f, bottomCenter.y / 2.0f);

  std::vector<MagmaModel::Vertex> leftVertices{
      {{preVertices[0].position}, preVertices[0].color},
      {{leftCenter}, preVertices[1].color},
      {{bottomCenter}, preVertices[2].color},
  };
  std::vector<MagmaModel::Vertex> topVertices{
      {{leftCenter}, preVertices[0].color},
      {{preVertices[1].position}, preVertices[1].color},
      {{rightCenter}, preVertices[2].color},
  };
  std::vector<MagmaModel::Vertex> rightVertices{
      {{bottomCenter}, preVertices[0].color},
      {{rightCenter}, preVertices[1].color},
      {{preVertices[2].position}, preVertices[2].color},
  };

  calculateSiepinskiTriangle(leftVertices, result, counter + 1);
  calculateSiepinskiTriangle(topVertices, result, counter + 1);
  calculateSiepinskiTriangle(rightVertices, result, counter + 1);
}

void Magma::createPipelineLayout() {
  VkPipelineLayoutCreateInfo pipelineLayoutInfo;
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;
  pipelineLayoutInfo.pNext = nullptr;
  pipelineLayoutInfo.flags = 0;

  if (vkCreatePipelineLayout(magmaDevice.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout");
  };
}

void Magma::createPipeline() {
  auto pipelineConfig = MagmaPipeline::defaultPipelineConfigInfo(
      magmaSwapChain.width(), magmaSwapChain.height());
  pipelineConfig.renderPass = magmaSwapChain.getRenderPass();
  pipelineConfig.pipelineLayout = pipelineLayout;
  magmaPipeline = std::make_unique<MagmaPipeline>(
      magmaDevice, "shaders/simple_shader.vert.spv",
      "shaders/simple_shader.frag.spv", pipelineConfig);
}

void Magma::createCommandBuffers() {
  commandBuffers.resize(magmaSwapChain.imageCount());

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = magmaDevice.getCommandPool();
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(magmaDevice.device(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("failed to create command buffer!");
  }

  for (int i = 0; i < commandBuffers.size(); i++) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("failed to begin recording command buffer!");
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = magmaSwapChain.getRenderPass();
    renderPassInfo.framebuffer = magmaSwapChain.getFrameBuffer(i);

    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = magmaSwapChain.getSwapChainExtent();

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.1f, 0.1f, 0.1f, 0.1f};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    magmaPipeline->bind(commandBuffers[i]);
    magmaModel->bind(commandBuffers[i]);
    magmaModel->draw(commandBuffers[i]);

    vkCmdEndRenderPass(commandBuffers[i]);
    if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("failed to record command buffer!");
    }
  }
}

void Magma::drawFrame() {
  uint32_t imageIndex;
  auto result = magmaSwapChain.acquireNextImage(&imageIndex);

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
    throw std::runtime_error("failed to acquire swap chain image!");
  }

  result = magmaSwapChain.submitCommandBuffers(&commandBuffers[imageIndex],
                                               &imageIndex);
  if (result != VK_SUCCESS) {
    throw std::runtime_error("failed to present swap chain image");
  }
}

} // namespace magma
