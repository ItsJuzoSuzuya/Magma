
#include "simple_render_system.hpp"
#include "magma_device.hpp"
#include "magma_game_object.hpp"
#include "magma_pipeline.hpp"
#include <GLFW/glfw3.h>
#include <cassert>
#include <glm/common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace magma {

struct SimplePushConstantData {
  glm::mat4 transform{1.f};
  alignas(16) glm::vec3 color;
};

SimpleRenderSystem::SimpleRenderSystem(MagmaDevice &device,
                                       VkRenderPass renderPass)
    : magmaDevice{device} {
  createPipelineLayout();
  createPipeline(renderPass);
}

SimpleRenderSystem::~SimpleRenderSystem() {
  vkDestroyPipelineLayout(magmaDevice.device(), pipelineLayout, nullptr);
}

void SimpleRenderSystem::createPipelineLayout() {

  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(SimplePushConstantData);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo;
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 0;
  pipelineLayoutInfo.pSetLayouts = nullptr;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
  pipelineLayoutInfo.pNext = nullptr;
  pipelineLayoutInfo.flags = 0;

  if (vkCreatePipelineLayout(magmaDevice.device(), &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("failed to create pipeline layout");
  };
}

void SimpleRenderSystem::createPipeline(VkRenderPass renderPass) {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before pipeline layout");

  PipelineConfigInfo pipelineConfig{};
  MagmaPipeline::defaultPipelineConfigInfo(pipelineConfig);
  pipelineConfig.renderPass = renderPass;
  pipelineConfig.pipelineLayout = pipelineLayout;
  magmaPipeline = std::make_unique<MagmaPipeline>(
      magmaDevice, "shaders/simple_shader.vert.spv",
      "shaders/simple_shader.frag.spv", pipelineConfig);
}

void SimpleRenderSystem::renderGameObjects(
    VkCommandBuffer commandBuffer, std::vector<MagmaGameObject> &gameObjects) {
  magmaPipeline->bind(commandBuffer);

  for (auto &obj : gameObjects) {
    obj.transform.rotation.y =
        glm::mod(obj.transform.rotation.y + 0.01f, glm::two_pi<float>());
    obj.transform.rotation.x =
        glm::mod(obj.transform.rotation.x + 0.005f, glm::two_pi<float>());

    SimplePushConstantData push{};
    push.color = obj.color;
    push.transform = obj.transform.mat4();

    vkCmdPushConstants(commandBuffer, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(SimplePushConstantData), &push);
    obj.model->bind(commandBuffer);
    obj.model->draw(commandBuffer);
  }
}

} // namespace magma
