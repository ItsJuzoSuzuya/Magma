module;
#include <GLFW/glfw3.h>
#include <functional>
#include <memory>
#include <print>
#include <vector>
#if defined(MAGMA_WITH_EDITOR)
  #include "imgui.h"
  #include "imgui_impl_vulkan.h"
  #include "imgui_impl_glfw.h"
#endif

export module engine:magma_engine;
import core;
import render;
import :render_system;
import :project_creator;
import :project;
import :scene_manager;

namespace Magma {

/**
 * The main engine class that initializes and runs the application.
 */
export class Engine {
public:
  Engine(Window *window): window{window} {
    setupRenderSystem();

    project = ProjectCreator::initProject();
    SceneManager::activeScene = project.scene.get();
    SceneManager::activeCamera = project.camera;

    // Provide render system with scene proxy collection callback
    SceneRenderer::getSceneProxies = []() -> std::vector<RenderProxy> {
      std::vector<RenderProxy> proxies;
      auto *scene = SceneManager::activeScene;
      if (!scene) return proxies;
      for (auto &go : scene->getGameObjects())
        if (go) proxies.push_back(go->collectProxies());
      return proxies;
    };

    std::println("Engine initialized successfully.");
  }

  void setupRenderSystem(){
    renderSystem = std::make_unique<RenderSystem>(*window);
  }

  void addRenderer(std::unique_ptr<IRenderer> renderer){
    renderSystem->addRenderer(std::move(renderer));
  }

  /** Set to receive dock/draw callbacks from editor setup (editor mode only) */
  std::function<void()> onEditorDock;

  void run() {
    std::println("Starting main loop...");
    while (!window->shouldClose()) {
      glfwPollEvents();
      if (glfwGetKey(window->getGLFWwindow(), GLFW_KEY_ESCAPE))
        window->close();

      #if defined(MAGMA_WITH_EDITOR)
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (onEditorDock) onEditorDock();
      #endif

      renderSystem->onRender();
    }
  }

private:
  Window *window = nullptr;
  Project project;

  std::unique_ptr<RenderSystem> renderSystem = nullptr;
};

} // namespace Magma
