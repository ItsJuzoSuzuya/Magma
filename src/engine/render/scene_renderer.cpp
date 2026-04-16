#include "engine/render/scene_renderer.hpp"
#include "core/device.hpp"
#include "core/frame_info.hpp"
#include "core/image_transitions.hpp"
#include "core/push_constant_data.hpp"
#include "core/render_proxy.hpp"
#include "core/render_target.hpp"
#include "core/renderer.hpp"
#include "engine/components/camera.hpp"
#include "engine/components/point_light.hpp"
#include "engine/render/features/object_picker.hpp"
#include "engine/render/render_callback.hpp"
#include "engine/render/swapchain_target.hpp"
#include "engine/scene_manager.hpp"
#include "render_context.hpp"
#include <cassert>
#include <cstdint>
#include <memory>
#include <print>
#include <utility>
#include <vector>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

#if defined(MAGMA_WITH_EDITOR)
  #include "imgui_impl_vulkan.h"
  #include "offscreen_target.hpp"
#endif

namespace Magma {

SceneRenderer::SceneRenderer(std::unique_ptr<IRenderTarget> target,
                             PipelineShaderInfo &shaderInfo)
    : IRenderer(), shaderInfo{shaderInfo} {
  renderTarget = std::move(target);

  if (auto *swapchainTarget = dynamic_cast<SwapchainTarget*>(renderTarget.get()))
    isSwapChainDependentFlag = true;

  // Per-renderer camera UBO — one buffer + descriptor set per frame in flight
  cameraLayout = DescriptorSetLayout::Builder()
      .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
      .build();

  cameraPool = DescriptorPool::Builder()
      .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
      .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
      .build();

  for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    cameraUBOs[i] = std::make_unique<Buffer>(
        sizeof(CameraUBO), 1,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    cameraUBOs[i]->map();

    VkDescriptorBufferInfo info{};
    info.buffer = cameraUBOs[i]->getBuffer();
    info.offset = 0;
    info.range  = sizeof(CameraUBO);

    DescriptorWriter(*cameraLayout, *cameraPool)
        .writeBuffer(0, &info)
        .build(cameraDescriptorSets[i]);
  }
}

SceneRenderer::~SceneRenderer() {
  destroy();
}

// ----------------------------------------------------------------------------
// Public Methods
// ----------------------------------------------------------------------------

void SceneRenderer::destroy() {
  Device::waitIdle();

  for (auto &buf : cameraUBOs) buf.reset();
  cameraLayout.reset();
  cameraPool.reset();   // frees pool and all sets allocated from it

  pipeline.reset();

  if (pipelineLayout != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(Device::get().device(), pipelineLayout, nullptr);
    pipelineLayout = VK_NULL_HANDLE;
  }
}

void SceneRenderer::addRenderFeature(std::unique_ptr<RenderFeature> feature){
  renderFeatures.push_back(std::move(feature));
}

void SceneRenderer::onResize(const VkExtent2D newExtent) {
  Device::waitIdle();

  #if defined(MAGMA_WITH_EDITOR)
    for (auto &tex : sceneTextures)
      ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)tex);
  #endif

  renderTarget->onResize(newExtent);
  for (auto &feature : renderFeatures)
    feature->onResize(newExtent);

  createPipeline();

  #if defined(MAGMA_WITH_EDITOR)
    sceneTextures.clear();
    createSceneTextures();
  #endif
}

void SceneRenderer::onRender() {
  begin();
  record();

  PointLightSSBO ssbo{};
  auto &gos = SceneManager::activeScene->getGameObjects();
  for (auto &go : gos) {
    if (cameraSource == CameraSource::Scene && go.get() == SceneManager::activeScene->activeCamera) 
      go->getComponent<Camera>()->onUpdate();

    auto proxy = go->collectProxies();
    if (proxy.pointLight && ssbo.lightCount < 128) {
      ssbo.lights[ssbo.lightCount++] = {
          proxy.pointLight->position,
          proxy.pointLight->color
      };
    }
    if (cameraSource == CameraSource::Scene && proxy.camera && go.get() == SceneManager::activeScene->activeCamera) {
      uploadCameraUBO({proxy.camera->projView});
    }
  }
  renderContext->updatePointLights(FrameInfo::frameIndex, &ssbo, sizeof(ssbo));

  if (cameraSource == CameraSource::Editor && editorCameraProxy.camera) 
    uploadCameraUBO({editorCameraProxy.camera->projView});

  for (auto &proxy : getSceneProxies())
    submitProxy(proxy);

  end();
}

SwapChain* SceneRenderer::getSwapChain() const {
  #if defined(MAGMA_WITH_EDITOR)
    if (auto *swapchainTarget = dynamic_cast<SwapchainTarget*>(renderTarget.get()))
      return swapchainTarget->swapChain();
    else
      return nullptr;
  #else
    return nullptr;
  #endif
}

std::vector<RenderProxy> SceneRenderer::getSceneProxies(){
  std::vector<RenderProxy> proxies;
  if (!SceneManager::activeScene) return proxies;
  for (auto &go : SceneManager::activeScene->getGameObjects())
    proxies.push_back(go->collectProxies());
  return proxies;
}

#if defined(MAGMA_WITH_EDITOR)
ImVec2 SceneRenderer::getSceneSize() const {
  return ImVec2(static_cast<float>(renderTarget->extent().width),
                static_cast<float>(renderTarget->extent().height));
}

ImTextureID SceneRenderer::getSceneTexture(size_t index) const {
  assert(index >= 0 &&
         index < renderTarget->imageCount() &&
         "SceneRenderer: Invalid image index in FrameInfo!");

  return sceneTextures.at(index);
}
#endif

// Rendering
void SceneRenderer::begin() {
  if (FrameInfo::commandBuffer == VK_NULL_HANDLE)
    throw std::runtime_error("No command buffer found in FrameInfo!");
  if (FrameInfo::frameIndex < 0 ||
      FrameInfo::frameIndex >= SwapChain::MAX_FRAMES_IN_FLIGHT)
    throw std::runtime_error("Invalid frame index in FrameInfo!");

  const uint32_t idx = FrameInfo::frameIndex;

  // Transition scene color image to COLOR_ATTACHMENT_OPTIMAL
  VkImageLayout colorLayout = renderTarget->getColorImageLayout(idx);
  if (colorLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    renderTarget->transitionColorImage(
        idx, ImageTransition::ShaderReadToColorOptimal);
  else if (colorLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    renderTarget->transitionColorImage(
        idx, ImageTransition::UndefinedToColorOptimal);

  VkImageLayout depthLayout = renderTarget->getDepthImageLayout(idx);
  if (depthLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    renderTarget->transitionDepthImage(
        idx, ImageTransition::UndefinedToDepthOptimal);

  for (auto &feature : renderFeatures)
    feature->prepare(idx);

  std::vector<VkRenderingAttachmentInfo> colors{};
  colors.emplace_back(renderTarget->getColorAttachment(idx));
  for (auto &feature : renderFeatures)
    feature->pushColorAttachments(colors, idx);

  VkRenderingAttachmentInfo depth = renderTarget->getDepthAttachment(idx);

  VkRenderingInfo renderingInfo = {};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = renderTarget->extent();
  renderingInfo.colorAttachmentCount = colors.size();
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

  VkDescriptorSet sets[] = {
      cameraDescriptorSets[FrameInfo::frameIndex],
      renderContext->getDescriptorSet(LayoutKey::PointLight, FrameInfo::frameIndex)};

  vkCmdBindDescriptorSets(FrameInfo::commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, getPipelineLayout(),
                          0, 2, sets, 0, nullptr);
}

void SceneRenderer::end() {
  if (FrameInfo::commandBuffer == VK_NULL_HANDLE)
    throw std::runtime_error("No command buffer found in FrameInfo!");
  if (FrameInfo::frameIndex < 0 ||
      FrameInfo::frameIndex >= SwapChain::MAX_FRAMES_IN_FLIGHT)
    throw std::runtime_error("Invalid frame index in FrameInfo!");

  vkCmdEndRendering(FrameInfo::commandBuffer);

  const uint32_t idx = FrameInfo::frameIndex;
  #if defined(MAGMA_WITH_EDITOR)
    // Transition scene color to SHADER_READ_ONLY for ImGui sampling
    renderTarget->transitionColorImage(
        idx, ImageTransition::ColorOptimalToShaderRead);
  #else
    renderTarget->transitionColorImage(
        idx, ImageTransition::ColorOptimalToPresent);
  #endif

  for (auto &feature : renderFeatures)
    feature->finish(idx);
}

void SceneRenderer::submitProxy(const RenderProxy &proxy) {
  if (proxy.transform) RenderCallback::renderTransform(*this, *proxy.transform);
  if (proxy.mesh) RenderCallback::renderMesh(*proxy.mesh);
}

void SceneRenderer::uploadCameraUBO(const CameraUBO &ubo) {
  cameraUBOs[FrameInfo::frameIndex]->writeToBuffer((void *)&ubo, sizeof(ubo));
}

// Textures
#if defined(MAGMA_WITH_EDITOR)
  void SceneRenderer::createSceneTextures() {
    assert(renderTarget != nullptr &&
           "Cannot create scene textures for null render target!");

    sceneTextures.resize(renderTarget->imageCount());
    for (uint32_t i = 0; i < sceneTextures.size(); ++i) {
      VkSampler sampler = renderTarget->getColorSampler();
      VkImageView view  = renderTarget->getColorImageView(i);

      sceneTextures[i] = (ImTextureID)ImGui_ImplVulkan_AddTexture(
          sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    }
  }
#endif


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

  uint32_t colorAttachmentCount = renderTarget->getColorAttachmentCount() + renderFeatures.size();
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

  pipeline.reset();
  pipeline = make_unique<Pipeline>(shaderInfo.vertFile, shaderInfo.fragFile, pipelineConfigInfo);
}

} // namespace Magma
