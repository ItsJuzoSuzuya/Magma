#pragma once
#include "../engine/render/imgui_renderer.hpp"
#include "../engine/render/offscreen_renderer.hpp"
#include "../engine/editor_camera.hpp"
#include "device.hpp"
#include "frame_info.hpp"
#include "imgui_impl_vulkan.h"
#include "renderer.hpp"
#include "swapchain.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Magma {

class Window;

class RenderSystem {
public:
  RenderSystem(Window &window);
  ~RenderSystem();

  // Getters
  #if defined (MAGMA_WITH_EDITOR)
  ImGui_ImplVulkan_InitInfo getImGuiInitInfo();
  #endif
  SwapChain &getSwapChain() { return *swapChain; }


  // Render
  void renderFrame();

private:
  /** Vulkan Logical Device */
  std::unique_ptr<Device> device = nullptr;
  Window &window;

  // SwapChain
  std::unique_ptr<SwapChain> swapChain = nullptr;
  void recreateSwapChain();

  // Renderering
  std::unique_ptr<OffscreenRenderer> offscreenRenderer = nullptr;
  #if defined (MAGMA_WITH_EDITOR)
  std::unique_ptr<ImGuiRenderer> imguiRenderer = nullptr;
  #endif
  bool beginFrame();
  void endFrame();

  // Resize
  void onWindowResized();
  void onSceneResize();

  // Command buffers
  std::vector<VkCommandBuffer> commandBuffers;
  VkCommandBuffer imageAcquireCommandBuffer = VK_NULL_HANDLE;
  void createCommandBuffers();

  // ImGui Dockspace
  void createDockspace(ImGuiID &dockspace_id, const ImVec2 &size);

  // Frame info
  FrameInfo frameInfo;
  bool firstFrame = true;

  // Editor Camera
  #if defined (MAGMA_WITH_EDITOR)
  std::unique_ptr<EditorCamera> editorCamera = nullptr;
  #endif

  // FPS 
  double lastTime = 0.0f;
  double deltaTime = 0.0f;
  void calculateFPS(float deltaTime);
};
} // namespace Magma
