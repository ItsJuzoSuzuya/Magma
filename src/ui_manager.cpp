#include "ui_manager.hpp"
#include "engine/gameobject.hpp"
#include "engine/render/offscreen_target.hpp"
#include "engine/render/scene_renderer.hpp"
#include "engine/widgets/file_browser.hpp"
#include "engine/widgets/game_editor.hpp"
#include "engine/widgets/game_view.hpp"
#include "engine/widgets/inspector.hpp"
#include "engine/widgets/runtime_control.hpp"
#include "engine/widgets/scene_tree.hpp"

namespace Magma {

std::unique_ptr<ImGuiRenderer> UI_Manager::setupUI(Window *window, WidgetManager *widgetManager){
  // Create scene renderers
  editorRenderer = createEditorRenderer();
  std::println("EditorRenderer");
  gameRenderer = createGameRenderer();
  std::println("GameRenderer");

  // Editor camera
  auto id = static_cast<GameObject::id_t>(UINT64_MAX);
  editorCameraObject = std::make_unique<GameObject>(id, "Editor Camera");
  editorCameraObject->addComponent<Transform>();
  auto *cam = editorCameraObject->addComponent<Camera>();
  cam->setPerspectiveProjection(glm::radians(90.f), 16.f / 9.f, 0.1f, 100.f);
  editorCamera = EditorCamera(editorCameraObject.get());

  // Widgets
  widgetManager->addWidget(std::make_unique<RuntimeControl>());
  widgetManager->addWidget(std::make_unique<SceneTree>());
  widgetManager->addWidget(std::make_unique<Inspector>());
  widgetManager->addWidget(std::make_unique<FileBrowser>());

  SceneRenderer *editorRendererPtr = editorRenderer.get();
  auto resizeCb = [editorRendererPtr](VkExtent2D *newExtent){
    editorRendererPtr->onResize(*newExtent);
  };
  auto editor = std::make_unique<GameEditor>(*editorRendererPtr, resizeCb);
  editor->initEditorCamera(editorCameraObject.get());
  widgetManager->addWidget(std::move(editor));
  widgetManager->addWidget(std::make_unique<GameView>(*gameRenderer.get()));

  // ImGui swapchain renderer
  PipelineShaderInfo imguiShaderInfo = {
    .vertFile = "src/shaders/shader.vert.spv",
    .fragFile = "src/shaders/imgui.frag.spv"
  };
  RenderTargetInfo rtInfo = {};
  auto swapchainTarget = std::make_unique<SwapchainTarget>(window->getExtent(), rtInfo);
  auto imguiRenderer =
    std::make_unique<ImGuiRenderer>(std::move(swapchainTarget), imguiShaderInfo);

  imguiRenderer->initImGui(*window);
  return imguiRenderer;
}

std::unique_ptr<SceneRenderer> UI_Manager::createEditorRenderer(){
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
  renderer->addRenderFeature(
      std::make_unique<ObjectPicker>(rtInfo.extent, rtInfo.imageCount));
  return renderer;
}

std::unique_ptr<SceneRenderer> UI_Manager::createGameRenderer(){
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
  return renderer;
}

}
