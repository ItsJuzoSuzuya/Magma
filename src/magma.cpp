#include "magma.hpp"
#include "core/magma_buffer.hpp"
#include "core/magma_camera.hpp"
#include "core/render/simple_render_system.hpp"
#include "keyboard_movement_controller.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

// std
#include <chrono>

namespace magma {

struct GlobalUbo {
  glm::mat4 projectionView{1.f};
  glm::vec3 lightDirection = glm::normalize(glm::vec3{1.f, -3.f, -1.f});
};

Magma::Magma() {
  globalPool = MagmaDescriptorPool::Builder(magmaDevice)
                   .setMaxSets(MagmaSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                MagmaSwapChain::MAX_FRAMES_IN_FLIGHT)
                   .build();
  loadGameObjects();
}

Magma::~Magma() {}

void Magma::run() {
  std::vector<std::unique_ptr<MagmaBuffer>> uboBuffers(
      MagmaSwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < uboBuffers.size(); i++) {
    uboBuffers[i] = std::make_unique<MagmaBuffer>(
        magmaDevice, sizeof(GlobalUbo), 1, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    uboBuffers[i]->map();
  }

  auto globalSetLayout = MagmaDescriptorSetLayout::Builder(magmaDevice)
                             .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                         VK_SHADER_STAGE_VERTEX_BIT)
                             .build();

  std::vector<VkDescriptorSet> globalDescriptorSets(
      MagmaSwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < globalDescriptorSets.size(); i++) {
    auto bufferInfo = uboBuffers[i]->descriptorInfo();
    MagmaDescriptorWriter(*globalSetLayout, *globalPool)
        .writeBuffer(0, &bufferInfo)
        .build(globalDescriptorSets[i]);
  }

  SimpleRenderSystem simpleRenderSystem{
      magmaDevice, magmaRenderer.getSwapChainRenderPass(),
      globalSetLayout->getDescriptorSetLayout()};
  MagmaCamera camera{};

  auto viewerObject = MagmaGameObject::createGameObject();
  KeyboardMovementController cameraController{};

  auto currentTime = std::chrono::high_resolution_clock::now();
  while (!magmaWindow.shouldClose()) {
    glfwPollEvents();

    auto newTime = std::chrono::high_resolution_clock::now();
    float frameTime =
        std::chrono::duration<float, std::chrono::seconds::period>(newTime -
                                                                   currentTime)
            .count();
    currentTime = newTime;

    cameraController.move(magmaWindow.getGLFWwindow(), frameTime, viewerObject);
    camera.setViewYXZ(viewerObject.transform.position,
                      viewerObject.transform.rotation);

    float aspect = magmaRenderer.getAspectRatio();
    camera.setPerspectiveProjection(glm::radians(50.f), aspect, 0.1f, 10.f);

    if (auto commandBuffer = magmaRenderer.beginFrame()) {

      int frameIndex = magmaRenderer.getFrameIndex();
      FrameInfo frameInfo{frameIndex, frameTime, commandBuffer, camera,
                          globalDescriptorSets[frameIndex]};

      // update
      GlobalUbo ubo{};
      ubo.projectionView = camera.getProjection() * camera.getView();
      uboBuffers[frameIndex]->writeToBuffer(&ubo);
      uboBuffers[frameIndex]->flush();

      // render
      magmaRenderer.beginSwapChainRenderPass(commandBuffer);
      simpleRenderSystem.renderGameObjects(frameInfo, gameObjects);
      magmaRenderer.endSwapChainRenderPass(commandBuffer);
      magmaRenderer.endFrame();
    }
  }

  vkDeviceWaitIdle(magmaDevice.device());
}

void Magma::loadGameObjects() {
  std::shared_ptr<MagmaModel> magmaModel =
      MagmaModel::createModelFromFile(magmaDevice, "models/flat_vase.obj");
  auto flatVase = MagmaGameObject::createGameObject();
  flatVase.model = magmaModel;
  flatVase.transform.position = {-.5f, .5f, 2.5f};
  flatVase.transform.scale = {3.f, 1.5f, 3.f};
  gameObjects.push_back(std::move(flatVase));

  magmaModel =
      MagmaModel::createModelFromFile(magmaDevice, "models/smooth_vase.obj");
  auto smoothVase = MagmaGameObject::createGameObject();
  smoothVase.model = magmaModel;
  smoothVase.transform.position = {.5f, .5f, 2.5f};
  smoothVase.transform.scale = {3.f, 1.5f, 3.f};
  gameObjects.push_back(std::move(smoothVase));
}

} // namespace magma
