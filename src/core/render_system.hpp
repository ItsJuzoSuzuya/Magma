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
  ImGui_ImplVulkan_InitInfo getImGuiInitInfo();
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
  std::unique_ptr<ImGuiRenderer> imguiRenderer = nullptr;
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
  std::unique_ptr<EditorCamera> editorCamera = nullptr;
};
} // namespace Magma
