#include "magma.hpp"
#include "core/buffer.hpp"
#include "core/game_object.hpp"
#include "core/model.hpp"
#include "core/occlusion_culler.hpp"
#include "core/render_system.hpp"
#include <GLFW/glfw3.h>
#include <chrono>
#include <glm/common.hpp>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define STB_IMAGE_IMPLEMENTATION

using namespace std;

namespace magma {

const int WIDTH = 1200;
const int HEIGHT = 800;

struct GlobalUbo {
  glm::mat4 projectionView{1.f};
  glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};
  glm::vec3 lightPosition{0.f, 20.f, 0.f};
  alignas(16) glm::vec4 lightColor{1.f};
};

Magma::Magma() {
  descriptorPool = DescriptorPool::Builder(device)
                       .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                    SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .build();
  loadGameObjects();
}

void Magma::run() {
  vector<unique_ptr<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < static_cast<int>(uboBuffers.size()); i++) {
    uboBuffers[i] = make_unique<Buffer>(device, sizeof(GlobalUbo), 1,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    uboBuffers[i]->map();
  }

  auto descriptorSetLayout =
      DescriptorSetLayout::Builder(device)
          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                      VK_SHADER_STAGE_ALL_GRAPHICS)
          .build();

  vector<VkDescriptorSet> descriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < static_cast<int>(descriptorSets.size()); i++) {
    auto bufferInfo = uboBuffers[i]->descriptorInfo();
    DescriptorWriter(*descriptorSetLayout, *descriptorPool)
        .writeBuffer(0, &bufferInfo)
        .build(descriptorSets[i]);
  }

  RenderSystem renderSystem{device, window,
                            descriptorSetLayout->getDescriptorSetLayout()};

  Camera camera{};
  camera.setPerspectiveProjection(glm::radians(90.f),
                                  renderSystem.getAspectRatio(), 0.1f, 1000.f);

  auto currentTime = chrono::high_resolution_clock::now();

  while (!window.shouldClose()) {
    /* GLFW Poll Events */

    glfwPollEvents();
    if (glfwGetKey(window.getGLFWwindow(), GLFW_KEY_ESCAPE))
      window.close();

    /* Delta Time */

    auto newTime = chrono::high_resolution_clock::now();
    float deltaTime =
        chrono::duration<float, chrono::seconds::period>(newTime - currentTime)
            .count();
    currentTime = newTime;

    /* Rendering */

    if (auto commandBuffer = renderSystem.beginFrame()) {
      int frameIndex = renderSystem.getFrameIndex();

      GlobalUbo ubo{};
      ubo.projectionView = camera.getProjection() * camera.getView();
      uboBuffers[frameIndex]->writeToBuffer(&ubo);
      uboBuffers[frameIndex]->flush();

      FrameInfo frameInfo{frameIndex, commandBuffer, camera,
                          descriptorSets[frameIndex]};

      renderSystem.recordCommandBuffer(commandBuffer);
      renderSystem.renderGameObjects(frameInfo, gameObjects);
      renderSystem.endRenderPass(commandBuffer);
      renderSystem.endFrame();
    }

    chrono::duration<double> frameTime =
        chrono::high_resolution_clock::now() - currentTime;
    // cout << "Frame Time: " << frameTime.count() * 1000 << "ms" << endl;
    // cout << "\rFps: " << 1.0 / frameTime.count() << flush;
    frameCounter++;
  }

  vkDeviceWaitIdle(device.device());
}

void Magma::loadGameObjects() {
  auto flatVase = GameObject::create();

  cout << "Adding Model" << endl;
  flatVase.addComponent<Model>(device, "models/barrel/Barrel.gltf");
  cout << "Adding Transform" << endl;
  flatVase.addComponent<Transform>();
  cout << "Pushing Object" << endl;
  gameObjects.push_back(std::move(flatVase));
}

} // namespace magma
