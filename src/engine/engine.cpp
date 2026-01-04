#include "engine.hpp"
#include "components/mesh.hpp"
#include "components/transform.hpp"
#include "core/render_target_info.hpp"
#include "core/renderer.hpp"
#include "engine/components/point_light.hpp"
#include "engine/render/imgui_renderer.hpp"
#include "engine/render/offscreen_target.hpp"
#include "engine/render/scene_renderer.hpp"
#include "gameobject.hpp"
#include "specifications.hpp"
#include <memory>
#include <print>
#include <GLFW/glfw3.h>
#include <X11/X.h>
#include <utility>
#include <vulkan/vulkan_core.h>

namespace Magma {

Engine::Engine(EngineSpecifications &spec) : specifications{spec} {
  window = std::make_unique<Window>(specifications);

  renderSystem = std::make_unique<RenderSystem>(*window);

  RenderTargetInfo rtInfo = {};
  std::unique_ptr<SwapchainTarget> swapchainTarget =
      std::make_unique<SwapchainTarget>(window->getExtent(), rtInfo);

  PipelineShaderInfo imguiShaderInfo = {
      .vertFile = "src/shaders/shader.vert.spv",
      .fragFile = "src/shaders/imgui.frag.spv"
  };
  auto imguiRenderer =
      std::make_unique<ImGuiRenderer>(std::move(swapchainTarget), imguiShaderInfo);
  renderSystem->addRenderer(std::move(imguiRenderer));

  rtInfo.extent = {800, 600};
  std::unique_ptr<OffscreenTarget> offscreenTarget =
      std::make_unique<OffscreenTarget>(rtInfo);

  PipelineShaderInfo sceneShaderInfo = {
      .vertFile = "src/shaders/shader.vert.spv",
      .fragFile = "src/shaders/editor.frag.spv"
  };
  auto sceneRenderer =
      std::make_unique<SceneRenderer>(std::move(offscreenTarget), sceneShaderInfo);
  renderSystem->addRenderer(std::move(sceneRenderer));

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
