#include "magma.hpp"
#include "magma_game_object.hpp"
#include "magma_pipeline.hpp"
#include <GLFW/glfw3.h>
#include <cassert>
#include <glm/common.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace magma {

struct SimplePushConstantData {
  glm::mat2 transform{1.f};
  glm::vec2 offset;
  alignas(16) glm::vec3 color;
};

Magma::Magma() {
  loadGameObjects();
  createPipelineLayout();
  createPipeline();
}

Magma::~Magma() {
  vkDestroyPipelineLayout(magmaDevice.device(), pipelineLayout, nullptr);
}

void Magma::run() {
  while (!magmaWindow.shouldClose()) {
    glfwPollEvents();

    if (auto commandBuffer = magmaRenderer.beginFrame()) {
      magmaRenderer.beginSwapChainRenderPass(commandBuffer);
      renderGameObjects(commandBuffer);
      magmaRenderer.endSwapChainRenderPass(commandBuffer);
      magmaRenderer.endFrame();
    }
  }

  vkDeviceWaitIdle(magmaDevice.device());
}

void Magma::loadGameObjects() {
  std::vector<MagmaModel::Vertex> vertices{
      {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
      {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
      {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
  };

  auto magmaModel = std::make_shared<MagmaModel>(magmaDevice, vertices);

  auto triangle = MagmaGameObject::createGameObject();
  triangle.model = magmaModel;
  triangle.color = {.0f, 1.f, .0f};
  triangle.transform2D.position.x = .2f;
  triangle.transform2D.scale = {2.f, 0.5f};
  triangle.transform2D.rotation = 0.5f * glm::pi<float>();

  gameObjects.push_back(std::move(triangle));
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

void Magma::createPipeline() {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before pipeline layout");

  PipelineConfigInfo pipelineConfig{};
  MagmaPipeline::defaultPipelineConfigInfo(pipelineConfig);
  pipelineConfig.renderPass = magmaRenderer.getSwapChainRenderPass();
  pipelineConfig.pipelineLayout = pipelineLayout;
  magmaPipeline = std::make_unique<MagmaPipeline>(
      magmaDevice, "shaders/simple_shader.vert.spv",
      "shaders/simple_shader.frag.spv", pipelineConfig);
}

void Magma::renderGameObjects(VkCommandBuffer commandBuffer) {
  magmaPipeline->bind(commandBuffer);

  for (auto &obj : gameObjects) {
    obj.transform2D.rotation =
        glm::mod(obj.transform2D.rotation + 0.01f, glm::two_pi<float>());

    SimplePushConstantData push{};
    push.offset = obj.transform2D.position;
    push.color = obj.color;
    push.transform = obj.transform2D.mat2();

    vkCmdPushConstants(commandBuffer, pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(SimplePushConstantData), &push);
    obj.model->bind(commandBuffer);
    obj.model->draw(commandBuffer);
  }
}

} // namespace magma
