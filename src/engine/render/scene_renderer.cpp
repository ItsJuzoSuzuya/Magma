#include "engine/render/scene_renderer.hpp"
#include "core/device.hpp"
#include "core/frame_info.hpp"
#include "core/image_transitions.hpp"
#include "core/push_constant_data.hpp"
#include "core/render_target.hpp"
#include "core/renderer.hpp"
#include "engine/components/camera.hpp"
#include "engine/render/swapchain_target.hpp"
#include "engine/scene.hpp"
#include "render_context.hpp"
#include <cstdint>
#include <print>
#include <utility>
#include <vector>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#if defined(MAGMA_WITH_EDITOR)
  #include "core/render_target_info.hpp"
  #include "imgui_impl_vulkan.h"
  #include "offscreen_target.hpp"
#endif

namespace Magma {

SceneRenderer::SceneRenderer(std::unique_ptr<IRenderTarget> target,
                             PipelineShaderInfo &shaderInfo)
    : IRenderer(), shaderInfo{shaderInfo} {
  renderContext = std::make_unique<RenderContext>();
  renderTarget = std::move(target);

  if (auto *offscreenTarget = dynamic_cast<SwapchainTarget*>(renderTarget.get())) 
    isSwapChainDependentFlag = true;

  // Self-register with the context to get our slice index
  rendererId = renderContext->registerRenderer();

  std::vector<VkDescriptorSetLayout> layouts = {
      renderContext->getLayout(LayoutKey::Camera),
      renderContext->getLayout(LayoutKey::PointLight)};

  createPipelineLayout(layouts);
  renderContext->createDescriptorSets(LayoutKey::Camera);
  renderContext->createDescriptorSets(LayoutKey::PointLight);

  createPipeline();
}

// ----------------------------------------------------------------------------
// Public Methods
// ----------------------------------------------------------------------------

void SceneRenderer::destroy() {
  Device::waitIdle();

  renderContext.reset();
  pipeline.reset();

  if (pipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(Device::get().device(), pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;
  }
}

void SceneRenderer::onResize(const VkExtent2D newExtent) {
  Device::waitIdle();

  renderTarget->onResize(newExtent);
  sceneColorLayouts.assign(renderTarget->imageCount(),
                           VK_IMAGE_LAYOUT_UNDEFINED);

  createPipeline();
}

void SceneRenderer::onRender() {
  begin();
  record();
  end();
}

ImVec2 SceneRenderer::getSceneSize() const {
  return ImVec2(static_cast<float>(renderTarget->extent().width),
                static_cast<float>(renderTarget->extent().height));
}

#if defined(MAGMA_WITH_EDITOR)
  int currentImageIndex() { return FrameInfo::frameIndex; }
#else
  int currentImageIndex() { return FrameInfo::imageIndex; }
#endif

// Rendering 
void SceneRenderer::begin() {
  if (FrameInfo::commandBuffer == VK_NULL_HANDLE)
    throw std::runtime_error("No command buffer found in FrameInfo!");
  if (FrameInfo::frameIndex < 0 ||
      FrameInfo::frameIndex >= SwapChain::MAX_FRAMES_IN_FLIGHT)
    throw std::runtime_error("Invalid frame index in FrameInfo!");

  const uint32_t idx = currentImageIndex();

  // Transition scene color image to COLOR_ATTACHMENT_OPTIMAL
  ImageTransitionDescription colorTransitionDesc = {};
  VkImageLayout colorLayout = renderTarget->getColorImageLayout(idx);
  if (colorLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
    colorTransitionDesc = ImageTransition::ShaderReadToColorOptimal;
  else if (colorLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) 
    colorTransitionDesc = ImageTransition::UndefinedToColorOptimal;
  renderTarget->transitionColorImage(idx, colorTransitionDesc);

  #if defined(MAGMA_WITH_EDITOR)
    // Transition ID image to COLOR_ATTACHMENT_OPTIMAL
    ImageTransitionDescription idTransitionDesc = {};
    VkImageLayout idLayout = objectPicker->getIdImageLayout(idx);
    if (idLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
      idTransitionDesc = ImageTransition::ShaderReadToColorOptimal;
    else if (idLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) 
      idTransitionDesc = ImageTransition::UndefinedToColorOptimal;
    objectPicker->transitionIdImage(idx, idTransitionDesc);
  #endif

  const uint32_t colorAttachmentCount = renderTarget->getColorAttachmentCount();
  std::vector<VkClearValue> clearValues(colorAttachmentCount + 1);
  clearValues[0].color = {{0.f, 0.f, 0.f, 1.f}};
  #if defined(MAGMA_WITH_EDITOR)
    clearValues[1].color = {};
    clearValues[1].color.uint32[0] = 0;
    clearValues[1].color.uint32[1] = 0;
    clearValues[1].color.uint32[2] = 0;
    clearValues[1].color.uint32[3] = 0;
  #endif
  clearValues[colorAttachmentCount].depthStencil = {1.0f, 0};

  VkRenderingAttachmentInfo color0 = {};
  color0.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  color0.imageView = renderTarget->getColorImageView(idx);
  color0.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color0.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color0.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color0.clearValue = clearValues[0];

  VkRenderingAttachmentInfo color1 = objectPicker->getIdAttachment(idx);

  VkRenderingAttachmentInfo depth = renderTarget->getDepthAttachment(idx);

  std::array<VkRenderingAttachmentInfo, 2> colors{};
  colors[0] = color0;
  #if defined(MAGMA_WITH_EDITOR)
    colors[1] = color1;
  #endif

  VkRenderingInfo renderingInfo = {};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = renderTarget->extent();
  renderingInfo.colorAttachmentCount = renderTarget->getColorAttachmentCount();
  renderingInfo.pColorAttachments = colors.data();
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

void SceneRenderer::record() {
  pipeline->bind(FrameInfo::commandBuffer);
  auto camSet =
      renderContext->getDescriptorSet(LayoutKey::Camera, FrameInfo::frameIndex);
  if (!camSet.has_value())
    throw std::runtime_error(
        "No Camera descriptor set found for OffscreenRenderer!");
  uint32_t camOffset = rendererId * renderContext->cameraSliceSize();

  auto lightSet = renderContext->getDescriptorSet(LayoutKey::PointLight,
                                                  FrameInfo::frameIndex);
  if (!lightSet.has_value())
    throw std::runtime_error(
        "No Point light descriptor set found for OffscreenRenderer!");

  VkDescriptorSet sets[] = {camSet.value(), lightSet.value()};
  uint32_t dynamicOffsets[] = {camOffset};

  vkCmdBindDescriptorSets(FrameInfo::commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, getPipelineLayout(),
                          0, 2, sets, 1, dynamicOffsets);
}

void SceneRenderer::end() {
  if (FrameInfo::commandBuffer == VK_NULL_HANDLE)
    throw std::runtime_error("No command buffer found in FrameInfo!");
  if (FrameInfo::frameIndex < 0 ||
      FrameInfo::frameIndex >= SwapChain::MAX_FRAMES_IN_FLIGHT)
    throw std::runtime_error("Invalid frame index in FrameInfo!");

  vkCmdEndRendering(FrameInfo::commandBuffer);

  const uint32_t idx = currentImageIndex();
  #if defined(MAGMA_WITH_EDITOR)
    // Transition scene color to SHADER_READ_ONLY for ImGui sampling
    VkImage sceneColor = renderTarget->getColorImage(idx);
    Device::transitionImageLayout(
        sceneColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
    sceneColorLayouts[idx] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    servicePendingPick();
  #else
    VkImage sceneColor = renderTarget->getColorImage(idx);
    Device::transitionImageLayout(
        sceneColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    sceneColorLayouts[idx] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  #endif
}

// Render Context Updates
void SceneRenderer::uploadCameraUBO(const CameraUBO &ubo) {
  if (!renderContext)
    return;
  renderContext->updateCameraSlice(FrameInfo::frameIndex, rendererId,
                                   (void *)&ubo, sizeof(ubo));
}

void SceneRenderer::submitPointLight(const PointLightData &lightData) {
  if (!renderContext)
    return;

  PointLightSSBO ssbo{};
  ssbo.lightCount = 1;
  ssbo.lights[0] = lightData;

  renderContext->updatePointLightSlice(FrameInfo::frameIndex, rendererId,
                                       (void *)&ssbo, sizeof(ssbo));
}

// ----------------------------------------------------------------------------
// Private Methods
// ----------------------------------------------------------------------------

void SceneRenderer::createPipelineLayout(
    const std::vector<VkDescriptorSetLayout> &descriptorSetLayouts) {
  std::vector<VkDescriptorSetLayout> layouts = descriptorSetLayouts;

  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstantData);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  VkDevice device = Device::get().device();
  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS)
    throw std::runtime_error("Failed to create pipeline layout!");
}

void SceneRenderer::createPipeline() {
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
