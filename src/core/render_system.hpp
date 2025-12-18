#pragma once
#include "../engine/render/offscreen_renderer.hpp"
#include "../engine/render/render_context.hpp"
#include "device.hpp"
#include "frame_info.hpp"
#include "renderer.hpp"
#include "swapchain.hpp"

#if defined(MAGMA_WITH_EDITOR)
#include "../engine/editor_camera.hpp"
#include "../engine/render/imgui_renderer.hpp"
#include "imgui_impl_vulkan.h"
#endif

#include <memory>
#include <vulkan/vulkan_core.h>

namespace Magma {

class Window;

class RenderSystem {
public:
  RenderSystem(Window &window);
  ~RenderSystem();

  #if defined(MAGMA_WITH_EDITOR)
    ImGui_ImplVulkan_InitInfo getImGuiInitInfo();
  #endif
  SwapChain &getSwapChain() { return *swapChain; }

  void renderFrame();

private:
  std::unique_ptr<Device> device = nullptr;
  Window &window;

  /** Swap chain 
   * Manages the presenttation to the window
   * Presents imgui in editor mode or the final rendered image in runtime mode
   * @note The swap chain must be recreated when the window is resized
   * */
  std::unique_ptr<SwapChain> swapChain = nullptr;
  void recreateSwapChain(VkExtent2D extent);
  #if defined(MAGMA_WITH_EDITOR)
    VkFormat imguiColorFormat = VK_FORMAT_UNDEFINED;
  #endif

  std::unique_ptr<RenderContext> renderContext = nullptr;
  #if defined(MAGMA_WITH_EDITOR)
    std::unique_ptr<OffscreenRenderer> offscreenRendererEditor = nullptr;
    std::unique_ptr<OffscreenRenderer> offscreenRendererGame = nullptr;
  #else
    std::unique_ptr<OffscreenRenderer> offscreenRenderer = nullptr;
  #endif

  bool beginFrame();
  void endFrame();

  void onWindowResize();
  void onSceneResize();

  std::vector<VkCommandBuffer> commandBuffers;
  VkCommandBuffer imageAcquireCommandBuffer = VK_NULL_HANDLE;
  void createCommandBuffers();

  #if defined(MAGMA_WITH_EDITOR)
    void createDockspace(ImGuiID &dockspace_id, const ImVec2 &size);
  #endif

  FrameInfo frameInfo;
  bool firstFrame = true;

// Editor
#if defined(MAGMA_WITH_EDITOR)
  std::unique_ptr<ImGuiRenderer> imguiRenderer = nullptr;
  std::unique_ptr<EditorCamera> editorCamera = nullptr;
#endif
};
} // namespace Magma
