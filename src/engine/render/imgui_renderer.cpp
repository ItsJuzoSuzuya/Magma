module;
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

export module render:imgui_renderer;
import core;

namespace Magma {

export class ImGuiRenderer : public IRenderer {
public:
  ImGuiRenderer(std::unique_ptr<SwapchainTarget> target, 
                PipelineShaderInfo shaderInfo): 
      IRenderer(), 
      shaderInfo{shaderInfo} {
    renderTarget = std::move(target);

    createDescriptorPool();
    createDescriptorSetLayout();
    std::vector<VkDescriptorSetLayout> descritporSetLayouts = {
        descriptorSetLayout->getDescriptorSetLayout()};
    createPipelineLayout(descritporSetLayouts);
    createPipeline();
  }

  std::function<void()> onDraw;

  void initImGui(const Window &window) {
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
      ImFont* font = io.Fonts->AddFontFromFileTTF(
          "assets/fonts/fa7-solid.otf", 10.0f, &config, fa_range);
      IM_ASSERT(font && "Failed to load fa6-solid.otf");
    }

    ImGui_ImplGlfw_InitForVulkan(window.getGLFWwindow(), true);
    ImGui_ImplVulkan_InitInfo init_info = getImGuiInitInfo();
    bool ok = ImGui_ImplVulkan_Init(&init_info);
    if (!ok)
      throw std::runtime_error(
          "Failed to initialize ImGui Vulkan implementation!");

    /*
    VkCommandBuffer cmd = Device::get().beginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture(cmd);
    Device::get().endSingleTimeCommands(cmd);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
    */
  }

  ~ImGuiRenderer() {
    vkDestroyPipelineLayout(Device::get().device(), pipelineLayout, nullptr);
  }
  void destroy() override {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  }

  VkDescriptorPool getDescriptorPool() const {
    return descriptorPool->getDescriptorPool();
  }
  SwapchainTarget &target() { return *renderTarget; }
  VkPipelineLayout getPipelineLayout() const override {
    return pipelineLayout; }


  void onResize(VkExtent2D extent) override {
    renderTarget->onResize(extent);
    createPipeline();
  }

  void onRender() override {
    begin();
    record();
    end();
  }

  bool isSwapChainDependent() const override { return true; }
  SwapChain* getSwapChain() const override {
    return renderTarget->swapChain(); }

private:
  std::unique_ptr<SwapchainTarget> renderTarget;

  VkFormat imguiColorFormat;
  ImGui_ImplVulkan_InitInfo getImGuiInitInfo() {
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

  // Rendering
  void begin() override {
    // Transition swapchain color image to COLOR_ATTACHMENT_OPTIMAL
    const uint32_t idx = FrameInfo::imageIndex;

    VkImageLayout current = renderTarget->getColorImageLayout(idx);
    if (current == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
      renderTarget->transitionColorImage(
          idx, ImageTransition::PresentToColorOptimal);
    else if (current != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
      renderTarget->transitionColorImage(
          idx, ImageTransition::UndefinedToColorOptimal);

    VkImageLayout depthCurrent =
        renderTarget->getDepthImageLayout(idx);
    if (depthCurrent != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
      renderTarget->transitionDepthImage(
          idx, ImageTransition::UndefinedToDepthOptimal);

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

  void record() override {
    pipeline->bind(FrameInfo::commandBuffer);
    onDraw();

    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    if (draw_data == nullptr) 
      return;


    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    FrameInfo::commandBuffer);

    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
      ImGui::UpdatePlatformWindows();
      ImGui::RenderPlatformWindowsDefault();
    }
  }

  void end() override {
    vkCmdEndRendering(FrameInfo::commandBuffer);

    // Transition swapchain image to PRESENT for presentation
    const uint32_t idx = FrameInfo::imageIndex;

    renderTarget->transitionColorImage(
        idx, ImageTransition::ColorOptimalToPresent);

  }

  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  void createPipelineLayout(
      const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts) override {

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

  std::unique_ptr<Pipeline> pipeline = nullptr;
  PipelineShaderInfo shaderInfo;
  void createPipeline() override {
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

  // Descriptor 
  std::unique_ptr<DescriptorPool> descriptorPool;
  std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
  void createDescriptorPool() {
    descriptorPool =
        DescriptorPool::Builder()
            .setMaxSets(100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
            .addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                         100 * SwapChain::MAX_FRAMES_IN_FLIGHT)
            .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
            .build();
  }
  void createDescriptorSetLayout() {
    descriptorSetLayout = DescriptorSetLayout::Builder()
                              .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                          VK_SHADER_STAGE_VERTEX_BIT)
                              .build();
  }

  // Widgets
  bool dockBuilt = false;
};

} // namespace Magma
