module engine:engine;
import std;
import x11;
import components;

export namespace Magma {

/**
 * The main engine class that initializes and runs the application.
 * It manages the window, rendering system, and scene.
 */
export class Engine {
public:
  Engine(EngineSpecifications &spec): specifications{spec} {
    window = std::make_unique<Window>(specifications);
    renderSystem = std::make_unique<RenderSystem>(*window);

    PipelineShaderInfo imguiShaderInfo = {
      .vertFile = "src/shaders/shader.vert.spv",
      .fragFile = "src/shaders/imgui.frag.spv"
    };
    PipelineShaderInfo editorShaderInfo = {
      .vertFile = "src/shaders/editor.vert.spv",
      .fragFile = "src/shaders/editor.frag.spv"
    };
    PipelineShaderInfo gameShaderInfo = {
      .vertFile = "src/shaders/shader.vert.spv",
      .fragFile = "src/shaders/shader.frag.spv"
    };

    RenderTargetInfo rtInfo = {};
    auto swapchainTarget = std::make_unique<SwapchainTarget>(window->getExtent(),
                                                             rtInfo);
    rtInfo.extent = {800, 600};
    auto offscreenTarget1 = std::make_unique<OffscreenTarget>(rtInfo);
    auto editorRenderer =
      std::make_unique<SceneRenderer>(std::move(offscreenTarget1), 
                                      editorShaderInfo);
    editorRenderer->cameraSource = CameraSource::Editor;
    editorRenderer->addRenderFeature(
        std::make_unique<ObjectPicker>(rtInfo.extent, rtInfo.imageCount));

    auto offscreenTarget2 = std::make_unique<OffscreenTarget>(rtInfo);
    auto gameRenderer = 
      std::make_unique<SceneRenderer>(std::move(offscreenTarget2), 
                                      gameShaderInfo);
    gameRenderer->cameraSource = CameraSource::Scene;

    auto imguiRenderer = 
      std::make_unique<ImGuiRenderer>(std::move(swapchainTarget),
                                      imguiShaderInfo);
    imguiRenderer->addWidget(std::make_unique<RuntimeControl>());
    imguiRenderer->addWidget(std::make_unique<SceneTree>());
    imguiRenderer->addWidget(std::make_unique<Inspector>());
    imguiRenderer->addWidget(std::make_unique<FileBrowser>());

    imguiRenderer->addWidget(std::make_unique<GameEditor>(
        *editorRenderer.get()));
    imguiRenderer->addWidget(std::make_unique<GameView>(*gameRenderer.get()));
    imguiRenderer->initImGui(*window.get());
    renderSystem->addRenderer(std::move(imguiRenderer));
    renderSystem->addRenderer(std::move(editorRenderer));
    renderSystem->addRenderer(std::move(gameRenderer));

    scene = std::make_unique<Scene>();

    auto &obj = GameObject::create();
    obj.name = "Test Object";

    Transform *transform = obj.addComponent<Transform>();
    transform->position = {0.f, 0.f, 1.f};
    transform->scale = {0.1f, 0.1f, 0.1f};

    auto &lightObj = GameObject::create();
    lightObj.name = "Light Object";
    lightObj.addComponent<PointLight>();

    if (!obj.addComponent<Mesh>()->load("assets/cube/scene.gltf"))
      throw std::runtime_error("Failed to load model!");

    std::println("Engine initialized successfully.");
  }

  // ----------------------------------------------------------------------------
  // Public Methods
  // ----------------------------------------------------------------------------

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
  EngineSpecifications &specifications;
  std::unique_ptr<Window> window = nullptr;
  std::unique_ptr<RenderSystem> renderSystem = nullptr;
  std::unique_ptr<Scene> scene = nullptr;

  #if defined(MAGMA_WITH_EDITOR)
    void initImGui();
  #endif
};

} // namespace Magma

