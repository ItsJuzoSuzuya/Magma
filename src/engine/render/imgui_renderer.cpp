#include "imgui_renderer.hpp"
#include "../../core/frame_info.hpp"
#include "../widgets/dock_layout.hpp"
#include "../widgets/ui_context.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "swapchain_target.hpp"
#include <array>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

// Constructor
ImGuiRenderer::ImGuiRenderer(SwapChain &swapChain) : Renderer() {
  createDescriptorPool();
  createDescriptorSetLayout();
  Renderer::init(descriptorSetLayout->getDescriptorSetLayout());

  renderTarget = make_unique<SwapchainTarget>(swapChain);
  createPipeline(renderTarget.get(), "src/shaders/shader.vert.spv",
                 "src/shaders/imgui.frag.spv");
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
void ImGuiRenderer::preFrame() {
  UIContext::ensureInit();

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

    UIContext::TopBarDockId = layout.splitUp(0.05f);

    layout.finish();

    // After finishing, fetch node and set flags
    if (UIContext::TopBarDockId != 0) {
      if (ImGuiDockNode *node =
              ImGui::DockBuilderGetNode(UIContext::TopBarDockId))
        node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar |
                            ImGuiDockNodeFlags_NoWindowMenuButton |
                            ImGuiDockNodeFlags_NoCloseButton;
    }
    dockBuilt = true;
  }

  // Run pre-frame hooks
  for (auto &widget : widgets)
    widget->preFrame();
}

// Rendering
void ImGuiRenderer::begin() {
  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.f, 0.f, 0.f, 1.f}};
  clearValues[1].depthStencil = {1.0f, 0};

  // Dynamic rendering attachments
  VkRenderingAttachmentInfo color{};
  color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  color.imageView = renderTarget->getColorImageView(FrameInfo::imageIndex);
  color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color.clearValue = clearValues[0];

  VkRenderingAttachmentInfo depth{};
  depth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  depth.imageView = renderTarget->getDepthImageView(FrameInfo::imageIndex);
  depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depth.clearValue = clearValues[1];

  VkRenderingInfo renderingInfo{};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = renderTarget->extent();
  renderingInfo.colorAttachmentCount = 1;
  renderingInfo.pColorAttachments = &color;
  renderingInfo.pDepthAttachment = &depth;
  renderingInfo.layerCount = 1;

  vkCmdBeginRendering(FrameInfo::commandBuffer, &renderingInfo);

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

  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                  FrameInfo::commandBuffer);
  ImGuiIO &io = ImGui::GetIO();
  if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
  }
}

void ImGuiRenderer::end() { vkCmdEndRendering(FrameInfo::commandBuffer); }

// Resize
void ImGuiRenderer::resize(VkExtent2D extent, VkSwapchainKHR swapChain) {
  renderTarget->resize(extent, swapChain);
  createPipeline(renderTarget.get(), "src/shaders/shader.vert.spv",
                 "src/shaders/imgui.frag.spv");
}

// --- Private ---
// Descriptors
void ImGuiRenderer::createDescriptorPool() {
  descriptorPool =
      DescriptorPool::Builder()
          .setMaxSets(100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
          .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                       100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
          .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
          .build();
}

void ImGuiRenderer::createDescriptorSetLayout() {
  descriptorSetLayout = DescriptorSetLayout::Builder()
                            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                        VK_SHADER_STAGE_VERTEX_BIT)
                            .build();
}

} // namespace Magma
