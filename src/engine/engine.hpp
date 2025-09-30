#pragma once
#include "../core/render_system.hpp"
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
  // Engine Specifications
  EngineSpecifications &specifications;

  // GLFW Window
  std::unique_ptr<Window> window = nullptr;
  void initGlfw();

  // Render System
  std::unique_ptr<RenderSystem> renderSystem = nullptr;
  void initRenderSystem();

  // ImGui
  void initImGui();
};

} // namespace Magma
