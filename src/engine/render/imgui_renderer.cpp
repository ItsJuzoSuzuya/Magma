#include "imgui_renderer.hpp"
#include "core/device.hpp"
#include "core/frame_info.hpp"
#include "core/image_transitions.hpp"
#include "core/renderer.hpp"
#include "core/window.hpp"
#include "engine/widgets/dock_layout.hpp"
#include "engine/widgets/ui_context.hpp"
#include "core/pipeline.hpp"
#include "core/push_constant_data.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "swapchain_target.hpp"
#include <array>
#include <memory>
#include <print>
#include <vulkan/vulkan_core.h>

namespace Magma {

ImGuiRenderer::ImGuiRenderer(std::unique_ptr<SwapchainTarget> target,
    PipelineShaderInfo shaderInfo): IRenderer(), shaderInfo{shaderInfo} {
  renderTarget = std::move(target);

  createDescriptorPool();
  createDescriptorSetLayout();
  std::vector<VkDescriptorSetLayout> descritporSetLayouts = {
      descriptorSetLayout->getDescriptorSetLayout()};
  createPipelineLayout(descritporSetLayouts);
  createPipeline();
}

ImGuiRenderer::~ImGuiRenderer() {
  vkDestroyPipelineLayout(Device::get().device(), pipelineLayout, nullptr);
}

// ----------------------------------------------------------------------------
// Public Methods
// ----------------------------------------------------------------------------

void ImGuiRenderer::initImGui(const Window &window) {
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize = ImVec2(
      static_cast<float>(renderTarget->extent().width),
      static_cast<float>(renderTarget->extent().height));
  io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  ImGui::StyleColorsDark();

  io.Fonts->AddFontDefault();

  {
    ImFontConfig config;
    config.MergeMode = true;
    config.PixelSnapH = true;
    config.GlyphMinAdvanceX = 0.0f;

    static const ImWchar fa_range[] = {0xF000, 0xF8FF, 0};
    UIContext::IconFont = io.Fonts->AddFontFromFileTTF(
        "assets/fonts/fa7-solid.otf", 10.0f, &config, fa_range);
    IM_ASSERT(UIContext::IconFont && "Failed to load fa6-solid.otf");
  }

  ImGui_ImplGlfw_InitForVulkan(window.getGLFWwindow(), true);
  ImGui_ImplVulkan_InitInfo init_info = getImGuiInitInfo();
  bool ok = ImGui_ImplVulkan_Init(&init_info);
  if (!ok)
    throw std::runtime_error(
        "Failed to initialize ImGui Vulkan implementation!");
}

void ImGuiRenderer::destroy() {
  ImGui_ImplVulkan_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

VkDescriptorPool ImGuiRenderer::getDescriptorPool() const {
  return descriptorPool->getDescriptorPool();
}

void ImGuiRenderer::addWidget(std::unique_ptr<Widget> widget) {
  widgets.push_back(std::move(widget));
}


void ImGuiRenderer::onResize(VkExtent2D extent) {
  renderTarget->onResize(extent);
  createPipeline();
}

void ImGuiRenderer::onRender() {
  newFrame();
  preFrame();

  begin();
  record();
  end();
}


// ----------------------------------------------------------------------------
// Private Methods
// ----------------------------------------------------------------------------

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
  // Transition swapchain color image to COLOR_ATTACHMENT_OPTIMAL
  const uint32_t idx = FrameInfo::imageIndex;

  ImageTransitionDescription colorTransitionDesc = {};
  VkImageLayout current = renderTarget->getColorImageLayout(idx);
  if (current == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    colorTransitionDesc = ImageTransition::PresentToColorOptimal;
  else if (current != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    colorTransitionDesc = ImageTransition::UndefinedToColorOptimal;

  renderTarget->transitionColorImage(idx, colorTransitionDesc);

  // Dynamic rendering attachments
  VkRenderingAttachmentInfo color = renderTarget->getColorAttachment(idx);
  VkRenderingAttachmentInfo depth = renderTarget->getDepthAttachment(idx);

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

void ImGuiRenderer::end() {
  vkCmdEndRendering(FrameInfo::commandBuffer);

  // Transition swapchain image to PRESENT for presentation
  const uint32_t idx = FrameInfo::imageIndex;

  renderTarget->transitionColorImage(
      idx, ImageTransition::ColorOptimalToPresent);
}

ImGui_ImplVulkan_InitInfo ImGuiRenderer::getImGuiInitInfo() {
  ImGui_ImplVulkan_InitInfo init_info = {};
  Device::get().populateImGuiInitInfo(&init_info);
  init_info.ApiVersion = VK_API_VERSION_1_3;
  init_info.DescriptorPool = getDescriptorPool();
  init_info.DescriptorPoolSize = 0;

  init_info.RenderPass = VK_NULL_HANDLE;
  init_info.Subpass = 0;

  // Store color format for dynamic rendering
  // @note This is needed because ImGui doesnt store the color format correctly
  // on Arch Linux systems
  imguiColorFormat = renderTarget->getColorFormat();

  init_info.UseDynamicRendering = true;
  init_info.PipelineRenderingCreateInfo = {};
  init_info.PipelineRenderingCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
  init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
  init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &imguiColorFormat ;
  init_info.PipelineRenderingCreateInfo.depthAttachmentFormat =
      renderTarget->getDepthFormat();
  init_info.PipelineRenderingCreateInfo.stencilAttachmentFormat =
      VK_FORMAT_UNDEFINED;

  init_info.MinImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
  init_info.ImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = nullptr;

  return init_info;
}

void ImGuiRenderer::createDockspace(ImGuiID &dockspace_id, const ImVec2 &size) {
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

void ImGuiRenderer::createPipelineLayout(
    const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts) {

  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstantData);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
  pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  VkDevice device = Device::get().device();
  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout!");
}

void ImGuiRenderer::createPipeline() {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before pipeline layout!");
  assert(renderTarget != nullptr &&
         "Cannot create pipeline for null render target!");

  PipelineConfigInfo pipelineConfigInfo = {};
  Pipeline::defaultPipelineConfig(pipelineConfigInfo);
  pipelineConfigInfo.pipelineLayout = pipelineLayout;

  uint32_t colorAttachmentCount = renderTarget->getColorAttachmentCount();
  if (pipelineConfigInfo.colorBlendAttachments.size() < colorAttachmentCount) {
    auto first = pipelineConfigInfo.colorBlendAttachments.empty()
                     ? VkPipelineColorBlendAttachmentState{}
                     : pipelineConfigInfo.colorBlendAttachments[0];
    pipelineConfigInfo.colorBlendAttachments.resize(colorAttachmentCount,
                                                    first);
    pipelineConfigInfo.colorBlendInfo.attachmentCount =
        static_cast<uint32_t>(pipelineConfigInfo.colorBlendAttachments.size());
    pipelineConfigInfo.colorBlendInfo.pAttachments =
        pipelineConfigInfo.colorBlendAttachments.data();
  } else {
    pipelineConfigInfo.colorBlendInfo.attachmentCount =
        static_cast<uint32_t>(pipelineConfigInfo.colorBlendAttachments.size());
    pipelineConfigInfo.colorBlendInfo.pAttachments =
        pipelineConfigInfo.colorBlendAttachments.data();
  }

  pipelineConfigInfo.colorAttachmentFormats.clear();
  for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
    if (i == 0) {
      pipelineConfigInfo.colorAttachmentFormats.push_back(
          renderTarget->getColorFormat());
    } else {
      pipelineConfigInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32_UINT);
    }
  }
  pipelineConfigInfo.depthFormat = renderTarget->getDepthFormat();

  pipeline = make_unique<Pipeline>(shaderInfo.vertFile, shaderInfo.fragFile, pipelineConfigInfo);
}

} // namespace Magma
