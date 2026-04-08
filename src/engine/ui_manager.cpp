module;
#include <memory>
#include <utility>

export module engine:ui_manager;
import render;
import components;
import widget;

namespace Magma {

export class UI_Manager {
public:
  UI_Manager(){}

  std::unique_ptr<ImGuiRenderer> setupUI(Window *windowm, WidgetManager *widgetManager){
    PipelineShaderInfo imguiShaderInfo = {
      .vertFile = "src/shaders/shader.vert.spv",
      .fragFile = "src/shaders/imgui.frag.spv"
    };

    auto cameraObject = new GameObject(UINT32_MAX, "Editor Camera");
    cameraObject->addComponent<Transform>();
    auto camera = cameraObject->addComponent<Camera>();
    camera->setPerspectiveProjection(glm::radians(90.f), 16.f / 9.f, 0.1f, 100.f);

    widgetManager->addWidget(std::make_unique<RuntimeControl>());
    widgetManager->addWidget(std::make_unique<SceneTree>());
    widgetManager->addWidget(std::make_unique<Inspector>());
    widgetManager->addWidget(std::make_unique<FileBrowser>());

    auto editor = std::make_unique<GameEditor>(*editorRenderer.get(), renderer.onResize(newExtent));
    editor->initEditorCamera(&cameraObject);
    imguiRenderer->addWidget(std::move(editor));
    imguiRenderer->addWidget(std::make_unique<GameView>(*gameRenderer.get()));


    RenderTargetInfo rtInfo = {};
    auto swapchainTarget = std::make_unique<SwapchainTarget>(window->getExtent(),
                                                             rtInfo);
    auto imguiRenderer = 
      std::make_unique<ImGuiRenderer>(std::move(swapchainTarget),
                                      imguiShaderInfo);

    imguiRenderer->initImGui(*window.get());
    return std::move(imguiRenderer);
  }

  std::unique_ptr<SceneRenderer> setupEditorRenderer(){
    PipelineShaderInfo editorShaderInfo = {
      .vertFile = "src/shaders/editor.vert.spv",
      .fragFile = "src/shaders/editor.frag.spv"
    };

    auto target = std::make_unique<OffscreenTarget>(rtInfo);
    auto renderer =
      std::make_unique<SceneRenderer>(std::move(target), 
                                      editorShaderInfo);
    renderer->addRenderFeature(
        std::make_unique<ObjectPicker>(rtInfo.extent, rtInfo.imageCount));
    return std::move(renderer);
  }
  
  std::unique_ptr<SceneRenderer> setupGameRenderer(){
    PipelineShaderInfo gameShaderInfo = {
      .vertFile = "src/shaders/shader.vert.spv",
      .fragFile = "src/shaders/shader.frag.spv"
    };

    auto target = std::make_unique<OffscreenTarget>(rtInfo);
    auto renderer = 
      std::make_unique<SceneRenderer>(std::move(target), 
                                      gameShaderInfo);
    renderer->cameraSource = CameraSource::Scene;
    return std::move(renderer);
  }
};


// PipelineShaderInfo -> OffscreenTarget -> SceneRenderer(CameraSource::Editor)
// PipelineShaderInfo -> OffscreenTarget -> SceneRenderer(CameraSource::Scene)

}
