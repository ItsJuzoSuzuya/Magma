#include "magma.hpp"
#include "keyboard_movement_controller.hpp"
#include "magma_camera.hpp"
#include "magma_game_object.hpp"
#include "simple_render_system.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/common.hpp>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/constants.hpp>
#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace magma {

Magma::Magma() { loadGameObjects(); }

Magma::~Magma() {}

void Magma::run() {
  SimpleRenderSystem simpleRenderSystem{magmaDevice,
                                        magmaRenderer.getSwapChainRenderPass()};
  MagmaCamera camera{};
  camera.setViewTarget(glm::vec3{.0f}, glm::vec3{.0f, .0f, 2.5f});

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
    camera.setPerspectiveProjection(glm::radians(50.f), aspect, .1f, 10.f);

    if (auto commandBuffer = magmaRenderer.beginFrame()) {
      magmaRenderer.beginSwapChainRenderPass(commandBuffer);
      simpleRenderSystem.renderGameObjects(commandBuffer, gameObjects, camera);
      magmaRenderer.endSwapChainRenderPass(commandBuffer);
      magmaRenderer.endFrame();
    }
  }

  vkDeviceWaitIdle(magmaDevice.device());
}

void Magma::loadGameObjects() {
  std::shared_ptr<MagmaModel> gameObjectModel =
      MagmaModel::createModelFromFile(magmaDevice, "./models/smooth_vase.obj");
  auto gameObject = MagmaGameObject::createGameObject();
  gameObject.model = gameObjectModel;
  gameObject.transform.position = {.0f, .0f, 2.5f};
  gameObject.transform.scale = glm::vec3{3.f};

  gameObjects.push_back(std::move(gameObject));
}

} // namespace magma
