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
  RenderTargetInfo rtInfo = {
    .extent = window->getExtent(),
    .colorFormat = VK_FORMAT_R8G8B8A8_UNORM,
    .depthFormat = VK_FORMAT_D32_SFLOAT,
    .imageCount = SwapChain::MAX_FRAMES_IN_FLIGHT
  };
  auto swapchainTarget = std::make_unique<SwapchainTarget>(rtInfo);
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
