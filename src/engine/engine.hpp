#pragma once
#include "../core/render_system.hpp"
#include "../core/window.hpp"
#include "scene.hpp"
#include <GLFW/glfw3.h>
#include <X11/X.h>
#include <memory>
namespace Magma {

class EngineSpecifications;

class Engine {
public:
  Engine(EngineSpecifications &spec);

  // Main loop
  void run();

private:
  // Engine
  EngineSpecifications &specifications;
  std::unique_ptr<Window> window = nullptr;
  std::unique_ptr<RenderSystem> renderSystem = nullptr;
  std::unique_ptr<Scene> scene = nullptr;

  // ImGui
  void initImGui();
};

} // namespace Magma
