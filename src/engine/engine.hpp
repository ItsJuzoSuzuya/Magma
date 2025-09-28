#pragma once
#include "../core/render_pipeline.hpp"
#include "../core/window.hpp"
#include <GLFW/glfw3.h>
#include <X11/X.h>
#include <memory>
namespace Magma {

class EngineSpecifications;

class Engine {
public:
  Engine(EngineSpecifications &spec);

  void run();

private:
  EngineSpecifications &specifications;

  // GLFW Window
  std::unique_ptr<Window> window;
  void initGlfw();

  // Vulkan Render Pipeline
  std::unique_ptr<RenderPipeline> renderPipeline;
  void initRenderPipeline();

  // ImGui
  void initImGui();
};

} // namespace Magma
