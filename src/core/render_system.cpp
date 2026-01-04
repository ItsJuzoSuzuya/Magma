#include "render_system.hpp"
#include "core/device.hpp"
#include "core/window.hpp"
#include "deletion_queue.hpp"
#include "engine/scene.hpp"
#include "engine/time.hpp"
#include "renderer.hpp"
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
  createCommandBuffers();
}

RenderSystem::~RenderSystem() {
  Device::waitIdle();

  renderContext.reset();
  destroyAllRenderers();

  DeletionQueue::flushAll();
}

// ----------------------------------------------------------------------------
// Public Methods
// ----------------------------------------------------------------------------

void RenderSystem::addRenderer(std::unique_ptr<IRenderer> renderer) { 
  renderers.push_back(std::move(renderer));
}

void RenderSystem::onRender() {
  Time::update(glfwGetTime());

  #if defined(MAGMA_WITH_EDITOR)
    if (!firstFrame) {
      for (auto &renderer : renderers) {
        if (auto *sceneRenderer = dynamic_cast<SceneRenderer*>(renderer.get())) 
          sceneRenderer->createSceneTextures();
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

// ----------------------------------------------------------------------------
// Private Methods
// ----------------------------------------------------------------------------

void RenderSystem::destroyAllRenderers() {
  for (auto &renderer : renderers)
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
  auto result = swapChain->acquireNextImage();
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
  for (auto &renderer : renderers) 
    renderer->onRender();
}

void RenderSystem::endFrame() {
  if (vkEndCommandBuffer(FrameInfo::commandBuffer) != VK_SUCCESS)
    throw std::runtime_error("Failed to record command buffer!");

  auto result = swapChain->submitCommandBuffer(&FrameInfo::commandBuffer);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      window.wasWindowResized()) {
    onWindowResize();
    return;
  } else if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to present swap chain image!");

  FrameInfo::advanceFrame();
  Scene::current()->processDeferredActions();
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
  for (auto &renderer : renderers){
    if (renderer->isSwapChainDependent())
      renderer->onResize(extent);
  }
}


} // namespace Magma
