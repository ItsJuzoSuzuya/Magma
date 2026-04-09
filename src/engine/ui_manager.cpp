module;
#include <functional>
#include <memory>
#include <print>
#include <utility>
#include <vulkan/vulkan_core.h>
#include <glm/trigonometric.hpp>

export module ui_manager;
import core;
import render;
import components;
import widgets;
import features;
import engine;

namespace Magma {

export class UI_Manager {
public:
  UI_Manager(){}

  std::unique_ptr<ImGuiRenderer> setupUI(Window *window, WidgetManager *widgetManager){
    // Create scene renderers
    editorRenderer_ = createEditorRenderer();
    std::println("EditorRenderer");
    gameRenderer_ = createGameRenderer();
    std::println("GameRenderer");

    // Editor camera
    auto id = static_cast<GameObject::id_t>(UINT64_MAX);
    editorCameraObject = std::make_unique<GameObject>(id, "Editor Camera");
    editorCameraObject->addComponent<Transform>();
    auto *cam = editorCameraObject->addComponent<Camera>();
    cam->setPerspectiveProjection(glm::radians(90.f), 16.f / 9.f, 0.1f, 100.f);
    editorCamera_ = EditorCamera(editorCameraObject.get());

    // Widgets
    widgetManager->addWidget(std::make_unique<RuntimeControl>());
    widgetManager->addWidget(std::make_unique<SceneTree>());
    widgetManager->addWidget(std::make_unique<Inspector>());
    widgetManager->addWidget(std::make_unique<FileBrowser>());

    SceneRenderer *editorRendererPtr = editorRenderer_.get();
    auto resizeCb = [editorRendererPtr](VkExtent2D *newExtent){
      editorRendererPtr->onResize(*newExtent);
    };
    auto editor = std::make_unique<GameEditor>(*editorRendererPtr, resizeCb);
    editor->initEditorCamera(editorCameraObject.get());
    widgetManager->addWidget(std::move(editor));
    widgetManager->addWidget(std::make_unique<GameView>(*gameRenderer_.get()));

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

  std::unique_ptr<SceneRenderer> setupEditorRenderer(){
    return std::move(editorRenderer_);
  }

  std::unique_ptr<SceneRenderer> setupGameRenderer(){
    return std::move(gameRenderer_);
  }

private:
  std::unique_ptr<SceneRenderer> editorRenderer_;
  std::unique_ptr<SceneRenderer> gameRenderer_;
  std::unique_ptr<GameObject> editorCameraObject;
  EditorCamera editorCamera_;

  std::unique_ptr<SceneRenderer> createEditorRenderer(){
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

  std::unique_ptr<SceneRenderer> createGameRenderer(){
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
};

} // namespace Magma
