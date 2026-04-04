module;
#include <memory>
#include <utility>
module engine:ui_manager;
import core:window;
import render:pipeline_shader_info;

namespace Magma {

export class UI_Manager {
public:
  UI_Manager(){}

  std::unique_ptr<ImGuiRenderer> setupUI(Window *window){
    PipelineShaderInfo imguiShaderInfo = {
      .vertFile = "src/shaders/shader.vert.spv",
      .fragFile = "src/shaders/imgui.frag.spv"
    };

    RenderTargetInfo rtInfo = {};
    auto swapchainTarget = std::make_unique<SwapchainTarget>(window->getExtent(),
                                                             rtInfo);
    auto imguiRenderer = 
      std::make_unique<ImGuiRenderer>(std::move(swapchainTarget),
                                      imguiShaderInfo);
    imguiRenderer->addWidget(std::make_unique<RuntimeControl>());
    imguiRenderer->addWidget(std::make_unique<SceneTree>());
    imguiRenderer->addWidget(std::make_unique<Inspector>());
    imguiRenderer->addWidget(std::make_unique<FileBrowser>());

    imguiRenderer->addWidget(std::make_unique<GameEditor>(*editorRenderer.get()));
    imguiRenderer->addWidget(std::make_unique<GameView>(*gameRenderer.get()));
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
