#include "engine.hpp"
#include "components/mesh.hpp"
#include "components/transform.hpp"
#include "gameobject.hpp"
#include "specifications.hpp"
#include <print>

#if defined(MAGMA_WITH_EDITOR)
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "widgets/ui_context.hpp"
#endif

#include <GLFW/glfw3.h>
#include <X11/X.h>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

// Constructor

Engine::Engine(EngineSpecifications &spec) : specifications{spec} {
  window = make_unique<Window>(specifications);
  renderSystem = make_unique<RenderSystem>(*window);

  #if defined(MAGMA_WITH_EDITOR)
    initImGui();
  #endif

  scene = make_unique<Scene>();

  auto &obj = GameObject::create();
  obj.name = "Test Object";

  Transform *transform = obj.addComponent<Transform>();
  transform->position = {0.f, 0.f, 1.f};
  transform->scale = {0.1f, 0.1f, 0.1f};

  if (!obj.addComponent<Mesh>()->load("assets/cube/scene.gltf")) 
    throw std::runtime_error("Failed to load model!");

  println("Engine initialized successfully.");
}

// --- Public ---
// Main loop
void Engine::run() {
  println("Starting main loop...");
  while (!window->shouldClose()) {
    glfwPollEvents();
    if (glfwGetKey(window->getGLFWwindow(), GLFW_KEY_ESCAPE))
      window->close();

    renderSystem->renderFrame();
  }
}

// --- Private ---
// Initialize ImGui
#if defined(MAGMA_WITH_EDITOR)
void Engine::initImGui() {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(static_cast<float>(specifications.windowWidth),
                          static_cast<float>(specifications.windowHeight));
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  ImGui::StyleColorsDark();

  io.Fonts->AddFontDefault();

  {
    ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = 0.0f;

    static const ImWchar fa_range[] = { 0xF000, 0xF8FF, 0 };
    UIContext::IconFont = io.Fonts->AddFontFromFileTTF(
        "assets/fonts/fa7-solid.otf", 10.0f, &config, fa_range);
    IM_ASSERT(UIContext::IconFont && "Failed to load fa6-solid.otf");
  }

  ImGui_ImplGlfw_InitForVulkan(window->getGLFWwindow(), true);
  ImGui_ImplVulkan_InitInfo init_info = renderSystem->getImGuiInitInfo();
  bool ok = ImGui_ImplVulkan_Init(&init_info);
  if (!ok)
    throw std::runtime_error(
        "Failed to initialize ImGui Vulkan implementation!");
}
#endif

} // namespace Magma
