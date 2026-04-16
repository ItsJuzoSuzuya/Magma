#pragma once
#include "core/render_system.hpp"
#include "core/window.hpp"
#include "engine/project.hpp"
#include "engine/render/imgui_renderer.hpp"
#include "engine/render/scene_renderer.hpp"
#include <memory>

namespace Magma {

/**
 * The main engine class that initializes and runs the application.
 * It manages the window, rendering system, and scene.
 */
class Engine {
public:
  Engine(Window &window);

  void setImguiRenderer(std::unique_ptr<ImGuiRenderer> renderer);
  SceneRenderer *createEditorRenderer();
  SceneRenderer *createGameRenderer();

  /**
   * Runs the main loop
   * @note This function will block until window is closed
   * by the user.
   */
  void run();

private:
  Window *window = nullptr;
  std::unique_ptr<RenderSystem> renderSystem = nullptr;
  Project project;

  void addSceneRenderer(std::unique_ptr<SceneRenderer> renderer);
};

} // namespace Magma
