#include "imgui_renderer.hpp"
#include "imgui_impl_glfw.h"
#include <array>

using namespace std;
namespace Magma {

// Constructor
ImGuiRenderer::ImGuiRenderer(Device &device, SwapChain &swapChain,
                             VkDescriptorSetLayout descriptorSetLayout)
    : Renderer(device, descriptorSetLayout) {
  renderTarget = make_unique<RenderTarget>(device, swapChain);
  createPipeline();
}

// Destructor
ImGuiRenderer::~ImGuiRenderer() {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

// Rendering
void ImGuiRenderer::begin(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.f, 0.f, 0.f, 1.f}};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo beginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  beginInfo.renderPass = renderTarget->getRenderPass();
  beginInfo.framebuffer = renderTarget->getFrameBuffer(frameIndex);
  beginInfo.renderArea.offset = {0, 0};
  beginInfo.renderArea.extent = renderTarget->extent();
  beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  beginInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = static_cast<float>(renderTarget->extent().height);
  viewport.width = static_cast<float>(renderTarget->extent().width);
  viewport.height = -static_cast<float>(renderTarget->extent().height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = renderTarget->extent();
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void ImGuiRenderer::record(VkCommandBuffer commandBuffer) {
  pipeline->bind(commandBuffer);
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

void ImGuiRenderer::end(VkCommandBuffer commandBuffer) {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
  vkCmdEndRenderPass(commandBuffer);
}

} // namespace Magma
