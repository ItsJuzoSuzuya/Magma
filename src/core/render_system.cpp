#include "render_system.hpp"
#include "core/device.hpp"
#include "core/frame_info.hpp"
#include "core/window.hpp"
#include "deletion_queue.hpp"
#include "engine/render/imgui_renderer.hpp"
#include "engine/render/scene_renderer.hpp"
#include "engine/scene.hpp"
#include "engine/scene_manager.hpp"
#include "engine/time.hpp"
#include "swapchain.hpp"
#include <vulkan/vulkan_core.h>
#include <GLFW/glfw3.h>
#include <cassert>
#include <memory>
#include "engine/widgets/game_view.hpp"

#if defined(MAGMA_WITH_EDITOR)
  #include "engine/widgets/game_editor.hpp"
#endif

namespace Magma {

RenderSystem::RenderSystem(Window &window) : window{window} {
  device = std::make_unique<Device>(window);
  renderContext = std::make_unique<RenderContext>();
  createCommandBuffers();
}

RenderSystem::~RenderSystem() {
  Device::waitIdle();

  // Destroy scenes before the device so mesh vertex/index buffers are
  // pushed to the DeletionQueue while it can still be flushed.
  SceneManager::scenes.clear();

  renderContext.reset();
  destroyAllRenderers();

  DeletionQueue::flushAll();
}

// ----------------------------------------------------------------------------
// Public Methods
// ----------------------------------------------------------------------------

void RenderSystem::setImGuiRenderer(std::unique_ptr<ImGuiRenderer> renderer) { 
  imguiRenderer = std::move(renderer);
}

void RenderSystem::addSceneRenderer(std::unique_ptr<SceneRenderer> renderer) { 
  renderer->initPipeline(renderContext.get());
  sceneRenderers.push_back(std::move(renderer));
}

void RenderSystem::onRender() {
  Time::update(glfwGetTime());

  #if defined(MAGMA_WITH_EDITOR)
    if (firstFrame) {
      for (auto &renderer : sceneRenderers) 
        renderer->createSceneTextures();
    }
  #endif

  if (beginFrame()) {
    renderFrame();
    endFrame();
  }

  if (firstFrame)
    firstFrame = false;
}

// ----------------------------------------------------------------------------
// Private Methods
// ----------------------------------------------------------------------------

void RenderSystem::destroyAllRenderers() {
  imguiRenderer->destroy();

  for (auto &renderer : sceneRenderers)
    renderer->destroy();
}

// Command Buffers
void RenderSystem::createCommandBuffers() {
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


// Rendering
bool RenderSystem::beginFrame() {
  VkResult result;
  #if defined(MAGMA_WITH_EDITOR)
    imguiRenderer->newFrame();
    imguiRenderer->preFrame();
    result = imguiRenderer->getSwapChain()->acquireNextImage();
  #else 
    for (auto &renderer : sceneRenderers) {
      if (renderer->isSwapChainDependent())
        result = renderer->getSwapChain()->acquireNextImage();
    }
  #endif

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

void RenderSystem::renderFrame() {
  for (auto &renderer : sceneRenderers) 
    renderer->onRender();

  #if defined (MAGMA_WITH_EDITOR)
    imguiRenderer->onRender();
  #endif
}

void RenderSystem::endFrame() {
  if (vkEndCommandBuffer(FrameInfo::commandBuffer) != VK_SUCCESS)
    throw std::runtime_error("Failed to record command buffer!");

  VkResult result;
  #if defined (MAGMA_WITH_EDITOR)
    result = imguiRenderer->getSwapChain()->submitCommandBuffer(&FrameInfo::commandBuffer);
  #else 
    result = renderers[0]->getSwapChain()->submitCommandBuffer(&FrameInfo::commandBuffer);
  #endif

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      window.wasWindowResized()) {
    onWindowResize();
    return;
  } else if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to present swap chain image!");

  FrameInfo::advanceFrame(SwapChain::MAX_FRAMES_IN_FLIGHT);
  if (SceneManager::activeScene) SceneManager::activeScene->processDeferredActions();
  DeletionQueue::flushForFrame(FrameInfo::frameIndex);
}

// Resize handling
void RenderSystem::onWindowResize() {
  auto extent = window.getExtent();
  while (extent.width == 0 || extent.height == 0) {
    extent = window.getExtent();
    glfwWaitEvents();
  }

  window.resetWindowResizedFlag();

  resizeSwapChainRenderer(extent);
}

void RenderSystem::resizeSwapChainRenderer(const VkExtent2D extent) {
  #if defined (MAGMA_WITH_EDITOR)
    imguiRenderer->onResize(extent);
  #else 
    renderers[0]->onResize(extent);
  #endif
}


} // namespace Magma
