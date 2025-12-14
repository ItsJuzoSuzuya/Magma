#pragma once
#include "../core/render_system.hpp"
#include "../core/window.hpp"
#include "scene.hpp"
#include <GLFW/glfw3.h>
#include <X11/X.h>
#include <memory>
namespace Magma {

class EngineSpecifications;

/**
 * The main engine class that initializes and runs the application.
 */
class Engine {
public:
  /** Constructs the engine with the given specifications */
  Engine(EngineSpecifications &spec);

  /**
   * Runs the main loop
   * @note This function will block until window is closed
   * by the user.
   */
  void run();

private:
  // --- Engine ---
  EngineSpecifications &specifications;
  std::unique_ptr<Window> window = nullptr;
  std::unique_ptr<RenderSystem> renderSystem = nullptr;
  std::unique_ptr<Scene> scene = nullptr;

  // --- ImGui --- 
  #if defined(MAGMA_WITH_EDITOR)
    void initImGui();
  #endif
};

} // namespace Magma
