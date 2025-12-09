#include "render_system.hpp"
#include "../engine/time.hpp"
#include "../core/window.hpp"
#include "../engine/scene.hpp"
#include "deletion_queue.hpp"
#include "device.hpp"
#include "renderer.hpp"
#include "swapchain.hpp"

#if defined(MAGMA_WITH_EDITOR)
#include "../engine/widgets/dock_layout.hpp"
#include "../engine/widgets/game_editor.hpp"
#include "../engine/widgets/game_view.hpp"
#include "../engine/widgets/inspector.hpp"
#include "../engine/widgets/runtime_control.hpp"
#include "../engine/widgets/scene_tree.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#endif

#include <GLFW/glfw3.h>
#include <cassert>
#include <memory>
#include <print>

using namespace std;
namespace Magma {

// Constructor
RenderSystem::RenderSystem(Window &window) : window{window} {
  device = make_unique<Device>(window);
  swapChain = make_unique<SwapChain>(window.getExtent());

#if defined(MAGMA_WITH_EDITOR)
  RenderTargetInfo offscreenInfo = swapChain->getRenderInfo();
  offscreenInfo.extent.width /= 2;
  offscreenInfo.extent.height /= 2;
  offscreenRenderer = make_unique<OffscreenRenderer>(offscreenInfo);
#else
  offscreenRenderer = make_unique<OffscreenRenderer>(*swapChain);
#endif

#if defined(MAGMA_WITH_EDITOR)
  editorCamera = make_unique<EditorCamera>();

  // Rendering ImGui
  imguiRenderer = make_unique<ImGuiRenderer>(*swapChain);

  // Add widgets
  imguiRenderer->addWidget(make_unique<RuntimeControl>());
  imguiRenderer->addWidget(make_unique<SceneTree>());
  imguiRenderer->addWidget(make_unique<Inspector>());

  // Important: GameEditor must be added last so that its content size is
  // calculated according to the other widgets
  imguiRenderer->addWidget(
      make_unique<GameEditor>(*offscreenRenderer.get(), editorCamera.get()));
  imguiRenderer->addWidget(make_unique<GameView>(*offscreenRenderer.get()));
#endif

  createCommandBuffers();
}

// Destructor
RenderSystem::~RenderSystem() {
#if defined(MAGMA_WITH_EDITOR)
  Device::waitIdle();
  DeletionQueue::flushAll();

  if (offscreenRenderer)
    offscreenRenderer.reset();

  ImGui_ImplVulkan_Shutdown();

  if (imguiRenderer)
    imguiRenderer.reset();

  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
#endif
}

// --- Public ---
// Getters
#if defined(MAGMA_WITH_EDITOR)
ImGui_ImplVulkan_InitInfo RenderSystem::getImGuiInitInfo() {
  ImGui_ImplVulkan_InitInfo init_info = {};
  device->populateImGuiInitInfo(&init_info);
  init_info.ApiVersion = VK_API_VERSION_1_3;
  init_info.DescriptorPool = imguiRenderer->getDescriptorPool();
  init_info.DescriptorPoolSize = 0;
  init_info.Subpass = 0;
  init_info.RenderPass = imguiRenderer->getRenderPass();
  init_info.MinImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
  init_info.ImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = nullptr;

  return init_info;
}
#endif

void RenderSystem::renderFrame() {
  // Create textures that will be displayed in an ImGui Image
  // Scene will be rendered to these textures in the offscreen pass

  Time::update(glfwGetTime());

#if defined(MAGMA_WITH_EDITOR)
  if (firstFrame)
    offscreenRenderer->createOffscreenTextures();
#endif

  if (beginFrame()) {
    offscreenRenderer->begin();
    offscreenRenderer->record();

#if defined(MAGMA_WITH_EDITOR)
    editorCamera->onUpdate();
    editorCamera->onRender(*offscreenRenderer);
#endif

    Scene::onRender(*offscreenRenderer);
    offscreenRenderer->end();

#if defined(MAGMA_WITH_EDITOR)
    imguiRenderer->begin();
    imguiRenderer->record();
    imguiRenderer->end();
#endif

    endFrame();
  }

  if (firstFrame)
    firstFrame = false;
}

// --- Private ---
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
    throw runtime_error("Failed to allocate command buffers!");
}

// Swap chain
void RenderSystem::recreateSwapChain(VkExtent2D extent) {
  Device::waitIdle();

  shared_ptr<SwapChain> oldSwapChain = std::move(swapChain);
  swapChain = make_unique<SwapChain>(extent, oldSwapChain);
}

// Rendering
bool RenderSystem::beginFrame() {
// Start ImGui frame
#if defined(MAGMA_WITH_EDITOR)
  imguiRenderer->newFrame();
  imguiRenderer->preFrame();
#endif

  // Check if the swap chain needs to be recreated
  auto result = swapChain->acquireNextImage();

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    onWindowResized();
    return false;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    throw runtime_error("Failed to acquire next swap chain image!");

  VkCommandBuffer commandBuffer = commandBuffers[FrameInfo::frameIndex];
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    throw runtime_error("Failed to begin recording command buffer!");

  FrameInfo::commandBuffer = commandBuffer;

  return true;
}

void RenderSystem::endFrame() {
  if (vkEndCommandBuffer(FrameInfo::commandBuffer) != VK_SUCCESS)
    throw runtime_error("Failed to record command buffer!");

  auto result = swapChain->submitCommandBuffer(&FrameInfo::commandBuffer);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      window.wasWindowResized()) {
    onWindowResized();
    return;
  } else if (result != VK_SUCCESS)
    throw runtime_error("Failed to present swap chain image!");

  FrameInfo::advanceFrame();
  Scene::current()->processDeferredActions();
  DeletionQueue::flushForFrame(FrameInfo::frameIndex);
}

// Resize
void RenderSystem::onWindowResized() {
  auto extent = window.getExtent();
  while (extent.width == 0 || extent.height == 0) {
    extent = window.getExtent();
    glfwWaitEvents();
  }

  window.resetWindowResizedFlag();
  recreateSwapChain(extent);

#if defined(MAGMA_WITH_EDITOR)
  imguiRenderer->resize(window.getExtent(), swapChain->getSwapChain());
#endif

#if defined(MAGMA_WITH_EDITOR)
  offscreenRenderer->resize(window.getExtent());
#else
  offscreenRenderer->resize(extent, swapChain->getSwapChain());
#endif
}

// ImGui Dockspace
#if defined(MAGMA_WITH_EDITOR)
void RenderSystem::createDockspace(ImGuiID &dockspace_id, const ImVec2 &size) {
  DockLayout dockLayout(dockspace_id, size);

  ImGuiID dock_id_left = dockLayout.splitLeft(0.25f);
  ImGuiID dock_id_right = dockLayout.splitRight(0.25f);
  ImGuiID dock_id_center = dockLayout.centerNode();

  dockLayout.makeCentral();

  dockLayout.dockWindow("Scene Tree", dock_id_left);
  dockLayout.dockWindow("Editor", dock_id_center);
  dockLayout.dockWindow("Inspector", dock_id_right);

  dockLayout.finish();
}
#endif

} // namespace Magma
