#include "render_pipeline.hpp"
#include "../core/window.hpp"
#include "../engine/widgets/dock_layout.hpp"
#include "buffer.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "imgui_internal.h"
#include "renderer.hpp"
#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <glm/mat4x4.hpp>
#include <memory>
#include <ranges>
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

RenderPipeline::RenderPipeline(Window &window) {
  device = make_unique<Device>(window);

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
  renderer = make_unique<Renderer>(*device, window,
                                   globalSetLayout->getDescriptorSetLayout());
}

// Destructor

RenderPipeline::~RenderPipeline() {
  vkDeviceWaitIdle(device->device());
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

// Getters

ImGui_ImplVulkan_InitInfo RenderPipeline::getImGuiInitInfo() {
  ImGui_ImplVulkan_InitInfo init_info = {};
  device->populateImGuiInitInfo(&init_info);
  init_info.ApiVersion = VK_API_VERSION_1_3;
  init_info.DescriptorPool = descriptorPool->getDescriptorPool();
  init_info.DescriptorPoolSize = 0;
  init_info.Subpass = 0;
  init_info.RenderPass = renderer->getSwapChain().getRenderPass();
  init_info.MinImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
  init_info.ImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = nullptr;

  return init_info;
}

// Render

void RenderPipeline::renderFrame() {
  if (firstFrame)
    renderer->createOffscreenTextures();

  if (auto commandBuffer = renderer->beginFrame()) {
    int frameIndex = renderer->getFrameIndex();

    FrameInfo frameInfo{frameIndex, commandBuffer,
                        globalDescriptorSets[frameIndex]};

    renderer->beginScene(commandBuffer);
    renderer->recordScene(commandBuffer);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    renderer->endScene(commandBuffer);

    {
      VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
      barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
      barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
      barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.image = renderer->getSceneImage();
      barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      barrier.subresourceRange.baseMipLevel = 0;
      barrier.subresourceRange.levelCount = 1;
      barrier.subresourceRange.baseArrayLayer = 0;
      barrier.subresourceRange.layerCount = 1;

      vkCmdPipelineBarrier(commandBuffer,
                           VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                           0, nullptr, 1, &barrier);
    }

    renderer->beginImGui(commandBuffer);
    renderer->recordImGui(commandBuffer);

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpaceOverViewport(dockspace_id, viewport);

    if (firstFrame)
      createDockspace(dockspace_id, viewport->Size);

    ImGui::Begin("Offscreen View");
    ImVec2 offscreenTargetSize{
        static_cast<float>(renderer->getSceneExtent().width),
        static_cast<float>(renderer->getSceneExtent().height)};

    ImGui::Image(renderer->getSceneTexture(), offscreenTargetSize);
    ImGui::End();

    ImGui::Begin("Scene Tree");
    ImGui::TreeNode("Settings");
    ImGui::End();

    ImGui::Begin("Inspector");
    ImGui::Text("Hello from the inspector!");
    ImGui::End();

    renderer->endImGui(commandBuffer);
    renderer->endFrame();
  }

  vkDeviceWaitIdle(device->device());
  if (firstFrame)
    firstFrame = false;
}

// Descriptor Pool
void RenderPipeline::createDescriptorPool() {
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
void RenderPipeline::createDockspace(ImGuiID &dockspace_id,
                                     const ImVec2 &size) {
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
