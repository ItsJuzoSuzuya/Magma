module;
#include <algorithm>
#include <memory>

module engine:magma_engine;
import :window;

namespace Magma {

/**
 * The main engine class that initializes and runs the application.
 * It manages the window, rendering system, and scene.
 */
export class Engine {
public:
  Engine(Window *window): window{window} {
    setupRenderSystem();

    #if defined (MAGMA_WITH_EDITOR)
      auto imguiRenderer = uiManger.setupUI();
      renderSystem->addRenderer(std::move(imguiRenderer))
    #endif

    auto gameRender = uiManger.setupGameRenderer();
    auto editorRenderer = uiManger.setupEditorRenderer();
    renderSystem->addRenderer(std::move(editorRenderer));
    renderSystem->addRenderer(std::move(gameRenderer));

    project = ProjectCreator::initProject();
    sceneManager.activeScene = project.scene.get();
    sceneManager.activeCamera = project.camera;

    std::println("Engine initialized successfully.");
  }

  // ----------------------------------------------------------------------------
  // Public Methods
  // ----------------------------------------------------------------------------

  void setupRenderSystem(){
    renderSystem = std::make_unique<RenderSystem>(*window);
  }

  void run() {
    std::println("Starting main loop...");
    while (!window->shouldClose()) {
      glfwPollEvents();
      if (glfwGetKey(window->getGLFWwindow(), GLFW_KEY_ESCAPE))
        window->close();

      renderSystem->onRender();
    }
  }

private:
  UI_Manager uiManger = {};
  Scene_Manager sceneManager = {};

  Window *window = nullptr;
  Project project;

  std::unique_ptr<RenderSystem> renderSystem = nullptr;
};



} // namespace Magma

