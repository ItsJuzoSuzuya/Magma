#include "engine.hpp"
#include "engine/project_creator.hpp"
#include "engine/render/imgui_renderer.hpp"
#include "engine/render/offscreen_target.hpp"
#include "engine/render/scene_renderer.hpp"
#include "engine/scene_manager.hpp"
#include <memory>
#include <print>
#include <GLFW/glfw3.h>

namespace Magma {

Engine::Engine(Window &window) : window(&window) {
  renderSystem = std::make_unique<RenderSystem>(window);

  project = ProjectCreator::initProject();
  SceneManager::activeScene = project.scene;
  SceneManager::activeScene->activeCamera = project.camera;

  std::println("Engine initialized successfully.");
}

// ----------------------------------------------------------------------------
// Public Methods
// ----------------------------------------------------------------------------

void Engine::setImguiRenderer(std::unique_ptr<ImGuiRenderer> renderer) {
  renderSystem->setImGuiRenderer(std::move(renderer));
}

SceneRenderer* Engine::createEditorRenderer(){
  PipelineShaderInfo editorShaderInfo = {
    .vertFile = "src/shaders/editor.vert.spv",
    .fragFile = "src/shaders/editor.frag.spv"
  };
  RenderTargetInfo rtInfo = {
    .extent = {1280, 720},
    .colorFormat = VK_FORMAT_R8G8B8A8_UNORM,
    .depthFormat = VK_FORMAT_D32_SFLOAT,
    .imageCount = SwapChain::MAX_FRAMES_IN_FLIGHT
  };
  auto target = std::make_unique<OffscreenTarget>(rtInfo);
  auto renderer = std::make_unique<SceneRenderer>(std::move(target), editorShaderInfo);
  renderer->cameraSource = CameraSource::Editor;
  renderer->addRenderFeature(
      std::make_unique<ObjectPicker>(rtInfo.extent, rtInfo.imageCount));
  auto rPtr = renderer.get();

  renderSystem->addSceneRenderer(std::move(renderer));
  return rPtr;
}

SceneRenderer* Engine::createGameRenderer(){
  PipelineShaderInfo gameShaderInfo = {
    .vertFile = "src/shaders/shader.vert.spv",
    .fragFile = "src/shaders/shader.frag.spv"
  };
  RenderTargetInfo rtInfo = {
    .extent = {1280, 720},
    .colorFormat = VK_FORMAT_R8G8B8A8_UNORM,
    .depthFormat = VK_FORMAT_D32_SFLOAT,
    .imageCount = SwapChain::MAX_FRAMES_IN_FLIGHT
  };
  auto target = std::make_unique<OffscreenTarget>(rtInfo);
  auto renderer = std::make_unique<SceneRenderer>(std::move(target), gameShaderInfo);
  renderer->cameraSource = CameraSource::Scene;
  auto rPtr = renderer.get();

  renderSystem->addSceneRenderer(std::move(renderer));
  return rPtr;
}

void Engine::run() {
  std::println("Starting main loop...");
  while (!window->shouldClose()) {
    glfwPollEvents();
    if (glfwGetKey(window->getGLFWwindow(), GLFW_KEY_ESCAPE))
      window->close();

    renderSystem->onRender();
  }
}

// ----------------------------------------------------------------------------
// Private Methods
// ----------------------------------------------------------------------------

} // namespace Magma
