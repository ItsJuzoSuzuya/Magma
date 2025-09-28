#include "engine.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"
#include "specifications.hpp"
#include <GLFW/glfw3.h>
#include <X11/X.h>
#include <cstdio>
#include <iterator>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

// Constructor

Engine::Engine(EngineSpecifications &spec) : specifications{spec} {
  initGlfw();
  initRenderPipeline();
  initImGui();
}

void Engine::initGlfw() { window = make_unique<Window>(specifications); }

void Engine::initRenderPipeline() {
  renderPipeline = make_unique<RenderPipeline>(*window);
}

void Engine::initImGui() {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.DisplaySize = ImVec2(static_cast<float>(specifications.windowWidth),
                          static_cast<float>(specifications.windowHeight));
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(window->getGLFWwindow(), true);
  ImGui_ImplVulkan_InitInfo init_info = renderPipeline->getImGuiInitInfo();
  bool ok = ImGui_ImplVulkan_Init(&init_info);
  if (!ok)
    throw std::runtime_error(
        "Failed to initialize ImGui Vulkan implementation!");
}

// Main loop

void Engine::run() {
  while (!window->shouldClose()) {
    glfwPollEvents();
    if (glfwGetKey(window->getGLFWwindow(), GLFW_KEY_ESCAPE))
      window->close();

    renderPipeline->renderFrame();
  }
}

} // namespace Magma
