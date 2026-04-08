module;
#include <vulkan/vulkan_core.h>
#include <vector>
#include <memory>
#include <GLFW/glfw3.h>

export module engine:render_system;
import core;
import :scene;


namespace Magma {

class RenderSystem {
public:
  RenderSystem(Window &window){
    device = std::make_unique<Device>(window);
    createCommandBuffers();
  }
  ~RenderSystem(){
    Device::waitIdle();

    renderContext.reset();
    destroyAllRenderers();

    DeletionQueue::flushAll();
  }

  void addRenderer(std::unique_ptr<IRenderer> renderer){
    renderers.push_back(std::move(renderer));
  }
  void onRender(){
    Time::update(glfwGetTime());

    #if defined(MAGMA_WITH_EDITOR)
      if (firstFrame) {
        for (auto &renderer : renderers) {
          if (auto *sceneRenderer = dynamic_cast<SceneRenderer*>(renderer.get())) {
            sceneRenderer->createSceneTextures();
          };
        }
      }
    #endif

    if (beginFrame()) {
      renderFrame();
      endFrame();
    }

    if (firstFrame)
      firstFrame = false;
  }

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

  std::vector<std::unique_ptr<IRenderer>> renderers;
  void resizeSwapChainRenderer(const VkExtent2D extent);

  void destroyAllRenderers(){
    for (auto &renderer : renderers)
      renderer->destroy();
  }

  bool beginFrame(){
    VkResult result;
    for (auto &renderer : renderers) {
      if (renderer->isSwapChainDependent())
        result = renderer->getSwapChain()->acquireNextImage();
    }

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      onWindowResize();
      return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR){
      throw std::runtime_error("Failed to acquire next swap chain image!");
    }

    VkCommandBuffer commandBuffer = commandBuffers[FrameInfo::frameIndex];
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
      throw std::runtime_error("Failed to begin recording command buffer!");

    FrameInfo::commandBuffer = commandBuffer;

    return true;
  }
  void renderFrame(){
    for (auto &renderer : renderers) 
      if (!renderer->isSwapChainDependent())
        renderer->onRender();

    for (auto &renderer : renderers)
      if (renderer->isSwapChainDependent())
        renderer->onRender();
  }
  void endFrame(){
    if (vkEndCommandBuffer(FrameInfo::commandBuffer) != VK_SUCCESS)
      throw std::runtime_error("Failed to record command buffer!");

    VkResult result;
    for (auto &renderer : renderers) 
      if (renderer->isSwapChainDependent())
        result = renderer->getSwapChain()->submitCommandBuffer(&FrameInfo::commandBuffer);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
        window.wasWindowResized()) {
      onWindowResize();
      return;
    } else if (result != VK_SUCCESS)
      throw std::runtime_error("Failed to present swap chain image!");

    FrameInfo::advanceFrame(SwapChain::MAX_FRAMES_IN_FLIGHT);
    Scene::current()->processDeferredActions();
    DeletionQueue::flushForFrame(FrameInfo::frameIndex);
  }

  void onWindowResize() {
    auto extent = window.getExtent();
    while (extent.width == 0 || extent.height == 0) {
      extent = window.getExtent();
      glfwWaitEvents();
    }

    window.resetWindowResizedFlag();

    resizeSwapChainRenderer(extent);
  }

  void resizeSwapChainRenderer(const VkExtent2D extent) {
    for (auto &renderer : renderers){
      if (renderer->isSwapChainDependent())
        renderer->onResize(extent);
    }
  }

  std::vector<VkCommandBuffer> commandBuffers;
  VkCommandBuffer imageAcquireCommandBuffer = VK_NULL_HANDLE;
  void createCommandBuffers(){
    commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = device->getCommandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device->device(), &allocInfo,
                                 commandBuffers.data()) != VK_SUCCESS)
      throw std::runtime_error("Failed to allocate command buffers!");
  }

  FrameInfo frameInfo;
  bool firstFrame = true;
};
} // namespace Magma
