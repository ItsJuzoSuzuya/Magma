#include "engine/ui.hpp"
#include "engine/widgets/file_browser.hpp"
#include "engine/widgets/inspector.hpp"
#include "engine/widgets/runtime_control.hpp"
#include "engine/widgets/scene_tree.hpp"

namespace Magma {

std::unique_ptr<ImGuiRenderer> UI::setup(Window *window, WidgetManager *widgetManager){
  widgetManager->addWidget(std::make_unique<RuntimeControl>());
  widgetManager->addWidget(std::make_unique<SceneTree>());
  widgetManager->addWidget(std::make_unique<Inspector>());
  widgetManager->addWidget(std::make_unique<FileBrowser>());

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
  imguiRenderer->onPreFrame = [widgetManager]() {
    widgetManager->dock();
  };
  imguiRenderer->onDraw = [widgetManager](){
    widgetManager->drawWidgets();
  };

  return imguiRenderer;
}



}
