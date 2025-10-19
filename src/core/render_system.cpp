#include "render_system.hpp"
#include "../core/window.hpp"
#include "../engine/scene.hpp"
#include "../engine/widgets/dock_layout.hpp"
#include "../engine/widgets/inspector.hpp"
#include "../engine/widgets/offscreen_view.hpp"
#include "../engine/widgets/scene_tree.hpp"
#include "buffer.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "renderer.hpp"
#include "swapchain.hpp"
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <glm/mat4x4.hpp>
#include <memory>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

// Constructor
RenderSystem::RenderSystem(Window &window) : window{window} {
  device = make_unique<Device>(window);
  swapChain = make_unique<SwapChain>(*device, window.getExtent());

  createCommandBuffers();
  createDescriptorPool();
  globalSetLayout = DescriptorSetLayout::Builder(device->device()).build();

  globalDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (size_t i = 0; i < globalDescriptorSets.size(); i++) {
    DescriptorWriter(*globalSetLayout, *descriptorPool)
        .build(globalDescriptorSets[i]);
  }

  RenderTargetInfo offscreenInfo = swapChain->getRenderInfo();
  offscreenInfo.extent.width /= 2;
  offscreenInfo.extent.height /= 2;

  imguiRenderer = make_unique<ImGuiRenderer>(*device, *swapChain);

  offscreenRenderer = make_unique<OffscreenRenderer>(
      *device, offscreenInfo, globalSetLayout->getDescriptorSetLayout());

  // Add widgets
  imguiRenderer->addWidget(make_unique<SceneTree>());
  imguiRenderer->addWidget(make_unique<Inspector>(device.get()));

  // Important: Offscreen view must be added last so that its content size is
  // calculated according to the other widgets
  imguiRenderer->addWidget(
      make_unique<OffscreenView>(*offscreenRenderer.get()));
}

// Destructor
RenderSystem::~RenderSystem() {
  vkDeviceWaitIdle(device->device());
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

// --- Public ---
// Getters
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

void RenderSystem::renderFrame() {
  // Create textures that will be displayed in an ImGui Image
  // Scene will be rendered to these textures in the offscreen pass
  if (firstFrame)
    offscreenRenderer->createOffscreenTextures();

  if (beginFrame()) {
    offscreenRenderer->begin();
    offscreenRenderer->record();
    Scene::onRender(*offscreenRenderer);
    offscreenRenderer->end();

    imguiRenderer->begin();
    imguiRenderer->record();
    imguiRenderer->end();

    endFrame();
  }

  vkDeviceWaitIdle(device->device());
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
void RenderSystem::recreateSwapChain() {
  auto extent = window.getExtent();
  while (extent.width == 0 || extent.height == 0) {
    extent = window.getExtent();
    glfwWaitEvents();
  }

  vkDeviceWaitIdle(device->device());

  shared_ptr<SwapChain> oldSwapChain = std::move(swapChain);
  swapChain = make_unique<SwapChain>(*device, extent, oldSwapChain);
}

// Rendering
bool RenderSystem::beginFrame() {
  // Start ImGui frame
  imguiRenderer->newFrame();

  if (!imguiRenderer->preFrame())
    return false;

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
}

// Resize
void RenderSystem::onWindowResized() {
  window.resetWindowResizedFlag();
  recreateSwapChain();
  imguiRenderer->resize(window.getExtent(), swapChain->getSwapChain());
}

// Descriptor Pool
void RenderSystem::createDescriptorPool() {
  descriptorPool = DescriptorPool::Builder(device->device())
                       .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                    SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .build();
}

// ImGui Dockspace
void RenderSystem::createDockspace(ImGuiID &dockspace_id, const ImVec2 &size) {
  DockLayout dockLayout(dockspace_id, size);

  ImGuiID dock_id_left = dockLayout.splitLeft(0.25f);
  ImGuiID dock_id_right = dockLayout.splitRight(0.25f);
  ImGuiID dock_id_center = dockLayout.centerNode();

  dockLayout.makeCentral();

  dockLayout.dockWindow("Scene Tree", dock_id_left);
  dockLayout.dockWindow("Offscreen View", dock_id_center);
  dockLayout.dockWindow("Inspector", dock_id_right);

  dockLayout.finish();
}

} // namespace Magma
