#pragma once
#include "engine/render/scene_renderer.hpp"
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

  void addRenderer(std::unique_ptr<IRenderer> renderer);
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

  std::vector<std::unique_ptr<IRenderer>> renderers;
  void resizeSwapChainRenderer(const VkExtent2D extent);

  /*
  #if defined(MAGMA_WITH_EDITOR)

    void initImGui();
    ImGui_ImplVulkan_InitInfo getImGuiInitInfo();

    void createDockspace(ImGuiID &dockspace_id, const ImVec2 &size);
  #endif
*/
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
