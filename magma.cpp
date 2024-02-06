#include "magma.hpp"
#include "magma_pipeline.hpp"
#include <GLFW/glfw3.h>
#include <memory>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace magma {

Magma::Magma() {
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
  }
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

void Magma::createCommandBuffers() {}
void Magma::drawFrame() {}

} // namespace magma
