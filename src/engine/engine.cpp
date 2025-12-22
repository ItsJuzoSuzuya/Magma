#include "engine.hpp"
#include "components/mesh.hpp"
#include "components/transform.hpp"
#include "engine/components/point_light.hpp"
#include "gameobject.hpp"
#include "specifications.hpp"
#include <print>
#include <GLFW/glfw3.h>
#include <X11/X.h>
#include <vulkan/vulkan_core.h>

namespace Magma {

Engine::Engine(EngineSpecifications &spec) : specifications{spec} {
  window = std::make_unique<Window>(specifications);
  renderSystem = std::make_unique<RenderSystem>(*window);
  scene = std::make_unique<Scene>();

  auto &obj = GameObject::create();
  obj.name = "Test Object";

  Transform *transform = obj.addComponent<Transform>();
  transform->position = {0.f, 0.f, 1.f};
  transform->scale = {0.1f, 0.1f, 0.1f};

  auto &lightObj = GameObject::create();
  lightObj.name = "Light Object";
  lightObj.addComponent<PointLight>();

  if (!obj.addComponent<Mesh>()->load("assets/cube/scene.gltf"))
    throw std::runtime_error("Failed to load model!");

  std::println("Engine initialized successfully.");
}

// ----------------------------------------------------------------------------
// Public Methods
// ----------------------------------------------------------------------------

void Engine::run() {
  std::println("Starting main loop...");
  while (!window->shouldClose()) {
    glfwPollEvents();
    if (glfwGetKey(window->getGLFWwindow(), GLFW_KEY_ESCAPE))
      window->close();

    renderSystem->onRender();
  }
}

} // namespace Magma
