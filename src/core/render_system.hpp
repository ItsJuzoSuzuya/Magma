#pragma once
#include "engine/render/offscreen_renderer.hpp"
#include "engine/render/render_context.hpp"
#include "device.hpp"
#include "frame_info.hpp"
#include "renderer.hpp"
#include "swapchain.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

#if defined(MAGMA_WITH_EDITOR)
  #include "engine/editor_camera.hpp"
  #include "engine/render/imgui_renderer.hpp"
  #include "imgui_impl_vulkan.h"
#endif

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

  void onRender();

private:
  Window &window;
  std::unique_ptr<Device> device = nullptr;
  std::unique_ptr<RenderContext> renderContext = nullptr;

  /** Swap chain 
   * Manages the presentation to the window
   * Presents imgui in editor mode or the final rendered image in runtime mode
   * @note The swap chain must be recreated when the window is resized
   * */
  std::unique_ptr<SwapChain> swapChain = nullptr;
  void recreateSwapChain(VkExtent2D extent);

  std::unique_ptr<OffscreenRenderer> offscreenRenderer = nullptr;
  #if defined(MAGMA_WITH_EDITOR)
    std::unique_ptr<EditorCamera> editorCamera = nullptr;
    std::unique_ptr<OffscreenRenderer> offscreenRendererEditor = nullptr;

    VkFormat imguiColorFormat = VK_FORMAT_UNDEFINED;
    std::unique_ptr<ImGuiRenderer> imguiRenderer = nullptr;
    void initImGui();
    void createDockspace(ImGuiID &dockspace_id, const ImVec2 &size);
  #endif
  void initializeRenderers();
  void destroyAllRenderers();

  bool beginFrame();
  void renderFrame();
  void endFrame();

  void onWindowResize();
  void resizeSwapChainRenderer();

  std::vector<VkCommandBuffer> commandBuffers;
  VkCommandBuffer imageAcquireCommandBuffer = VK_NULL_HANDLE;
  void createCommandBuffers();

  FrameInfo frameInfo;
  bool firstFrame = true;
};
} // namespace Magma
