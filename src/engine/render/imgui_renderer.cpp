#include "imgui_renderer.hpp"
#include "../../core/frame_info.hpp"
#include "imgui_impl_glfw.h"
#include <array>
#include <print>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

// Constructor
ImGuiRenderer::ImGuiRenderer(Device &device, SwapChain &swapChain,
                             VkDescriptorSetLayout descriptorSetLayout)
    : Renderer(device, descriptorSetLayout) {
  println("Initializing ImGuiRenderer");
  renderTarget = make_unique<RenderTarget>(device, swapChain);
  createPipeline();
}

// New Frame
void ImGuiRenderer::newFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

// Rendering
void ImGuiRenderer::begin() {
  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.f, 0.f, 0.f, 1.f}};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo beginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  beginInfo.renderPass = renderTarget->getRenderPass();
  beginInfo.framebuffer = renderTarget->getFrameBuffer(FrameInfo::imageIndex);
  beginInfo.renderArea.offset = {0, 0};
  beginInfo.renderArea.extent = renderTarget->extent();
  beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  beginInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(FrameInfo::commandBuffer, &beginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = static_cast<float>(renderTarget->extent().height);
  viewport.width = static_cast<float>(renderTarget->extent().width);
  viewport.height = -static_cast<float>(renderTarget->extent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(FrameInfo::commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = renderTarget->extent();
  vkCmdSetScissor(FrameInfo::commandBuffer, 0, 1, &scissor);
}

void ImGuiRenderer::record() { pipeline->bind(FrameInfo::commandBuffer); }

void ImGuiRenderer::end() {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                  FrameInfo::commandBuffer);
  vkCmdEndRenderPass(FrameInfo::commandBuffer);
}

// Resize
void ImGuiRenderer::resize(VkExtent2D extent, VkSwapchainKHR swapChain) {
  println("Resizing ImGuiRenderer");
  renderTarget->resize(extent, swapChain);
  createPipeline();
}

} // namespace Magma
