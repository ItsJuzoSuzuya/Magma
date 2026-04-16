#pragma once
#include "engine/render/imgui_renderer.hpp"
#include "engine/render/render_context.hpp"
#include "device.hpp"
#include "engine/render/scene_renderer.hpp"
#include "frame_info.hpp"
#include "renderer.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Magma {

class Window;

class RenderSystem {
public:
  RenderSystem(Window &window);
  ~RenderSystem();

  void setImGuiRenderer(std::unique_ptr<ImGuiRenderer> renderer);
  void addSceneRenderer(std::unique_ptr<SceneRenderer> renderer);
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
  void recreateSwapChain(VkExtent2D extent);

  std::unique_ptr<ImGuiRenderer> imguiRenderer;
  std::vector<std::unique_ptr<SceneRenderer>> sceneRenderers;
  void resizeSwapChainRenderer(const VkExtent2D extent);

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
