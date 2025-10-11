#include "imgui_renderer.hpp"
#include "../../core/frame_info.hpp"
#include "../widgets/dock_layout.hpp"
#include "imgui_impl_glfw.h"
#include <array>
#include <print>
#include <string>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

// Constructor
ImGuiRenderer::ImGuiRenderer(Device &device, SwapChain &swapChain)
    : Renderer(device) {
  createDescriptorPool();
  createDescriptorSetLayout();
  Renderer::init(descriptorSetLayout->getDescriptorSetLayout());

  renderTarget = make_unique<RenderTarget>(device, swapChain);
  createPipeline();
  println("Descriptor pool {}", (void *)descriptorPool->getDescriptorPool());
}

// --- Public ---
// Getters
VkDescriptorPool ImGuiRenderer::getDescriptorPool() const {
  return descriptorPool->getDescriptorPool();
}

// Widget management
void ImGuiRenderer::addWidget(std::unique_ptr<Widget> widget) {
  widgets.push_back(std::move(widget));
}

// New Frame
void ImGuiRenderer::newFrame() {
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
}

// Pre-frame: set up dockspace and let widgets run their pre-frame hooks
// Build dockspace and run widget pre-frame hooks (e.g., offscreen resize)
bool ImGuiRenderer::preFrame() {
  ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
  ImGui::DockSpaceOverViewport(dockspace_id, viewport);

  // Build once on first frame
  if (!dockBuilt) {
    DockLayout layout(dockspace_id, viewport->Size);
    layout.makeCentral();

    // Very simple mapping: honor dock hints if present
    ImGuiID leftId = 0, rightId = 0, upId = 0, downId = 0;
    for (auto &widget : widgets) {
      auto hint = widget->dockHint();
      if (!hint.has_value())
        continue;

      switch (hint->side) {
      case DockSide::Left:
        if (leftId == 0)
          leftId = layout.splitLeft(hint->ratio);
        layout.dockWindow(widget->name(), leftId);
        break;
      case DockSide::Right:
        if (rightId == 0)
          rightId = layout.splitRight(hint->ratio);
        layout.dockWindow(widget->name(), rightId);
        break;
      case DockSide::Up:
        if (upId == 0)
          upId = layout.splitUp(hint->ratio);
        layout.dockWindow(widget->name(), upId);
        break;
      case DockSide::Down:
        if (downId == 0)
          downId = layout.splitDown(hint->ratio);
        layout.dockWindow(widget->name(), downId);
        break;
      case DockSide::Center:
        layout.dockWindow(widget->name(), layout.centerNode());
        break;
      }
    }

    layout.finish();
    dockBuilt = true;
  }

  // Run pre-frame hooks
  bool ok = true;
  for (auto &widget : widgets) {
    ok = ok && widget->preFrame();
    if (!ok)
      break;
  }

  if (!ok) {
    ImGui::EndFrame();
    return false;
  }
  return true;
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

void ImGuiRenderer::record() {
  pipeline->bind(FrameInfo::commandBuffer);

  for (auto &widget : widgets)
    widget->draw();
}

void ImGuiRenderer::end() {
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                  FrameInfo::commandBuffer);
  vkCmdEndRenderPass(FrameInfo::commandBuffer);
}

// Resize
void ImGuiRenderer::resize(VkExtent2D extent, VkSwapchainKHR swapChain) {
  renderTarget->resize(extent, swapChain);
  createPipeline();
}

// --- Private ---
// Descriptors
void ImGuiRenderer::createDescriptorPool() {
  descriptorPool =
      DescriptorPool::Builder(device.device())
          .setMaxSets(100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
          .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
          .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
          .build();
}

void ImGuiRenderer::createDescriptorSetLayout() {
  descriptorSetLayout = DescriptorSetLayout::Builder(device.device()).build();
}

} // namespace Magma
