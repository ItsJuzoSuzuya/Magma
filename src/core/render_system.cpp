#include "render_system.hpp"
#include "../core/window.hpp"
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

struct GlobalUbo {
  glm::mat4 projectionView{1.f};
  glm::vec4 ambientLightColor{1.f, 1.f, 1.f, .02f};
  glm::vec3 lightPosition{0.f, 20.f, 0.f};
  alignas(16) glm::vec4 lightColor{1.f};
};

// Constructor
RenderSystem::RenderSystem(Window &window) : window{window} {
  device = make_unique<Device>(window);
  swapChain = make_unique<SwapChain>(*device, window.getExtent());

  createCommandBuffers();
  createDescriptorPool();

  uboBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (int i = 0; i < static_cast<int>(uboBuffers.size()); i++) {
    uboBuffers[i] = make_unique<Buffer>(*device, sizeof(GlobalUbo), 1,
                                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    uboBuffers[i]->map();
  }
  globalSetLayout = DescriptorSetLayout::Builder(device->device())
                        .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                    VK_SHADER_STAGE_ALL_GRAPHICS)
                        .build();

  globalDescriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (size_t i = 0; i < globalDescriptorSets.size(); i++) {
    auto bufferInfo = uboBuffers[i]->descriptorInfo();
    DescriptorWriter(*globalSetLayout, *descriptorPool)
        .writeBuffer(0, &bufferInfo)
        .build(globalDescriptorSets[i]);
  }

  RenderTargetInfo offscreenInfo = swapChain->getRenderInfo();
  offscreenInfo.extent.width /= 2;
  offscreenInfo.extent.height /= 2;

  imguiRenderer = make_unique<ImGuiRenderer>(
      *device, *swapChain, globalSetLayout->getDescriptorSetLayout());

  offscreenRenderer = make_unique<OffscreenRenderer>(
      *device, offscreenInfo, globalSetLayout->getDescriptorSetLayout());

  // Add widgets
  imguiRenderer->addWidget(make_unique<SceneTree>());
  imguiRenderer->addWidget(make_unique<Inspector>());

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
  init_info.DescriptorPool = descriptorPool->getDescriptorPool();
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
  if (firstFrame)
    offscreenRenderer->createOffscreenTextures();

  if (beginFrame()) {
    offscreenRenderer->begin();
    offscreenRenderer->record();
    vkCmdDraw(FrameInfo::commandBuffer, 3, 1, 0, 0);
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

  /*
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
  ImGui::DockSpaceOverViewport(dockspace_id, viewport);
  if (firstFrame)
    createDockspace(dockspace_id, viewport->Size);

  // Check if offscreen view needs to be resized
  ImGui::Begin("Scene Tree");
  ImGui::End();

  ImGui::Begin("Inspector");
  ImGui::End();
}

  bool offscreen_open = ImGui::Begin("Offscreen View");
  if (offscreen_open) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 desired{
        (float)ImMax(1, (int)avail.x),
        (float)ImMax(1, (int)avail.y),
    };

    ImVec2 current = offscreenRenderer->getSceneSize();
    bool needsResize = ((int)desired.x != (int)current.x) ||
                       ((int)desired.y != (int)current.y);

    if (needsResize && !firstFrame) {
      VkExtent2D newExtent{
          (uint32_t)desired.x,
          (uint32_t)desired.y,
      };

      offscreenRenderer->resize(newExtent);

      ImGui::End();
      ImGui::EndFrame();
      return false;
    }
  }
  ImGui::End();
  */

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
}

// Resize
void RenderSystem::onWindowResized() {
  window.resetWindowResizedFlag();
  recreateSwapChain();
  imguiRenderer->resize(window.getExtent(), swapChain->getSwapChain());
}

// Descriptor Pool
void RenderSystem::createDescriptorPool() {
  descriptorPool =
      DescriptorPool::Builder(device->device())
          .setMaxSets(100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
          .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                       SwapChain::MAX_FRAMES_IN_FLIGHT)
          .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
          .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
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
