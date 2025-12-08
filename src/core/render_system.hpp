#pragma once
#include "../engine/render/offscreen_renderer.hpp"
#include "device.hpp"
#include "frame_info.hpp"
#include "renderer.hpp"
#include "swapchain.hpp"

#if defined(MAGMA_WITH_EDITOR)
#include "../engine/editor_camera.hpp"
#include "../engine/render/imgui_renderer.hpp"
#endif

#include <memory>
#include <vulkan/vulkan_core.h>

namespace Magma {

class Window;

class RenderSystem {
public:
  RenderSystem(Window &window);
  ~RenderSystem();

// Getters
#if defined(MAGMA_WITH_EDITOR)
#include "imgui_impl_vulkan.h"
  ImGui_ImplVulkan_InitInfo getImGuiInitInfo();
#endif
  SwapChain &getSwapChain() { return *swapChain; }

  void renderFrame();

private:
  /** Vulkan Logical Device */
  std::unique_ptr<Device> device = nullptr;
  Window &window;

  // SwapChain
  std::unique_ptr<SwapChain> swapChain = nullptr;
  void recreateSwapChain(VkExtent2D extent);

// Renderering
#if defined(MAGMA_WITH_EDITOR)
  std::unique_ptr<OffscreenRenderer> offscreenRendererEditor = nullptr;
  std::unique_ptr<OffscreenRenderer> offscreenRendererGame = nullptr;
#else
  std::unique_ptr<OffscreenRenderer> offscreenRenderer = nullptr;
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
#if defined(MAGMA_WITH_EDITOR)
  void createDockspace(ImGuiID &dockspace_id, const ImVec2 &size);
#endif

  // Frame info
  FrameInfo frameInfo;
  bool firstFrame = true;

// Editor
#if defined(MAGMA_WITH_EDITOR)
  std::unique_ptr<ImGuiRenderer> imguiRenderer = nullptr;
  std::unique_ptr<EditorCamera> editorCamera = nullptr;
#endif

  // FPS
  double lastTime = 0.0f;
  double deltaTime = 0.0f;
  void calculateFPS(float deltaTime);
};
} // namespace Magma
