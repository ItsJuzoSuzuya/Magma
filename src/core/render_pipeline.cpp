#include "render_pipeline.hpp"
#include "../core/window.hpp"
#include "buffer.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "renderer.hpp"
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

// Getters

ImGui_ImplVulkan_InitInfo RenderPipeline::getImGuiInitInfo() {
  ImGui_ImplVulkan_InitInfo init_info = {};
  device->populateImGuiInitInfo(&init_info);
  init_info.ApiVersion = VK_API_VERSION_1_3;
  init_info.DescriptorPool = descriptorPool->getDescriptorPool();
  init_info.DescriptorPoolSize = 0;
  init_info.Subpass = 1;
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
  if (auto commandBuffer = renderer->beginFrame()) {
    int frameIndex = renderer->getFrameIndex();

    FrameInfo frameInfo{frameIndex, commandBuffer,
                        globalDescriptorSets[frameIndex]};

    renderer->recordCommandBuffer(commandBuffer);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::End();
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

    renderer->endRenderPass(commandBuffer);
    renderer->endFrame();
  }

  vkDeviceWaitIdle(device->device());
}

// Descriptor Pool
void RenderPipeline::createDescriptorPool() {
  descriptorPool = DescriptorPool::Builder(device->device())
                       .setMaxSets(100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                    SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                                    100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
                       .build();
}

} // namespace Magma
