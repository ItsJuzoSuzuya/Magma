#include "offscreen_renderer.hpp"
#include "../../core/device.hpp"
#include "../../core/frame_info.hpp"
#include "../components/camera.hpp"
#include "../scene.hpp"
#include "render_context.hpp"
#include <cstdint>

#if defined(MAGMA_WITH_EDITOR)
#include "offscreen_target.hpp"
#include "../../core/render_target_info.hpp"
#include "imgui_impl_vulkan.h"
#endif

#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

#if defined(MAGMA_WITH_EDITOR)
OffscreenRenderer::OffscreenRenderer(RenderTargetInfo &info, RenderContext *renderContext): Renderer(), renderContext{renderContext} { 
  rendererId = nextRendererId;
  nextRendererId++;

  VkDescriptorSetLayout layout =
      renderContext->getLayout(LayoutKey::Camera);
  Renderer::init(layout);
  renderContext->createDescriptorSets(LayoutKey::Camera);

  renderTarget = make_unique<OffscreenTarget>(info);
  createPipeline(renderTarget.get(), "src/shaders/shader.vert.spv",
                 "src/shaders/shader.frag.spv");

  sceneColorLayouts.assign(renderTarget->imageCount(),
                           VK_IMAGE_LAYOUT_UNDEFINED);
  idColorLayouts.assign(renderTarget->imageCount(), VK_IMAGE_LAYOUT_UNDEFINED);
}
#else
OffscreenRenderer::OffscreenRenderer(SwapChain &swapChain, RenderContext *renderContext) : Renderer(), renderContext{renderContext} {
  rendererId = nextRendererId;
  nextRendererId++;

  VkDescriptorSetLayout layout =
      renderContext->getLayout(LayoutKey::Camera);
  Renderer::init(layout);
  renderContext->createDescriptorSets(LayoutKey::Camera);

  renderTarget = std::make_unique<SwapchainTarget>(swapChain);

  createPipeline(renderTarget.get(), "src/shaders/shader.vert.spv",
                 "src/shaders/shader.frag.spv");

  sceneColorLayouts.assign(renderTarget->imageCount(),
                           VK_IMAGE_LAYOUT_UNDEFINED);
}
#endif

// Destructor
OffscreenRenderer::~OffscreenRenderer() {
#if defined(MAGMA_WITH_EDITOR)
  Device::waitIdle();
  for (auto texture : textures)
    ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)texture);
#endif
}

// Getters
VkImage &OffscreenRenderer::getSceneImage() const {
  return renderTarget->getColorImage(FrameInfo::frameIndex);
}

#if defined(MAGMA_WITH_EDITOR)
ImVec2 OffscreenRenderer::getSceneSize() const {
  return ImVec2(static_cast<float>(renderTarget->extent().width),
                static_cast<float>(renderTarget->extent().height));
}

// Textures
void OffscreenRenderer::createOffscreenTextures() {
  textures.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (uint32_t i = 0; i < textures.size(); ++i) {
    textures[i] = (ImTextureID)ImGui_ImplVulkan_AddTexture(
        renderTarget->getColorSampler(), renderTarget->getColorImageView(i),
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
}

// Picking: enqueue request and poll result
void OffscreenRenderer::requestPick(uint32_t x, uint32_t y) {
  pendingPick.hasRequest = true;
  pendingPick.x = x;
  pendingPick.y = y;
  // result will be produced after offscreen render ends (in servicePendingPick)
}

GameObject *OffscreenRenderer::pollPickResult() {
  GameObject *out = pendingPick.result;
  pendingPick.result = nullptr;
  return out;
}
#endif

#if defined(MAGMA_WITH_EDITOR)
static int currentImageIndex() { return FrameInfo::frameIndex; }
#else
static int currentImageIndex() { return FrameInfo::imageIndex; }
#endif

// Rendering helpers
void OffscreenRenderer::begin() {
  if (FrameInfo::commandBuffer == VK_NULL_HANDLE)
    throw runtime_error("No command buffer found in FrameInfo!");
  if (FrameInfo::frameIndex < 0 ||
      FrameInfo::frameIndex >= SwapChain::MAX_FRAMES_IN_FLIGHT)
    throw runtime_error("Invalid frame index in FrameInfo!");

  const uint32_t idx = currentImageIndex();

  // Transition scene color image to COLOR_ATTACHMENT_OPTIMAL
  VkImage sceneColor = renderTarget->getColorImage(idx);
  VkImageLayout curSceneLayout = sceneColorLayouts[idx];
  if (curSceneLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags srcAccess = 0;
    if (curSceneLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      srcAccess = VK_ACCESS_SHADER_READ_BIT;
    }

    Device::transitionImageLayout(
        sceneColor, curSceneLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        srcStage, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, srcAccess,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    sceneColorLayouts[idx] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  }

#if defined(MAGMA_WITH_EDITOR)
  // Transition ID image to COLOR_ATTACHMENT_OPTIMAL
  VkImage idImage =
      static_cast<OffscreenTarget *>(renderTarget.get())->getIdImage();
  VkImageLayout curIdLayout = idColorLayouts[idx];
  if (curIdLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkAccessFlags srcAccess = 0;
    if (curIdLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
      srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
      srcAccess = VK_ACCESS_SHADER_READ_BIT;
    }

    Device::transitionImageLayout(
        idImage, curIdLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        srcStage, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, srcAccess,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

    idColorLayouts[idx] = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  }
#endif

  const uint32_t colorAttachmentCount = renderTarget->getColorAttachmentCount();
  vector<VkClearValue> clearValues(colorAttachmentCount + 1);
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
  color0.imageView = renderTarget->getColorImageView(currentImageIndex());
  color0.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color0.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color0.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color0.clearValue = clearValues[0];

  VkRenderingAttachmentInfo color1{};
#if defined(MAGMA_WITH_EDITOR)
  color1.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  color1.imageView =
      static_cast<OffscreenTarget *>(renderTarget.get())->getIdImageView();
  color1.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  color1.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  color1.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  color1.clearValue = clearValues[1];
#endif

  VkRenderingAttachmentInfo depth = {};
  depth.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  depth.imageView = renderTarget->getDepthImageView(currentImageIndex());
  depth.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depth.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depth.clearValue = clearValues[colorAttachmentCount];

  std::array<VkRenderingAttachmentInfo, 2> colors{};
  colors[0] = color0;
#if defined(MAGMA_WITH_EDITOR)
  colors[1] = color1;
#endif

  VkRenderingInfo renderingInfo = {};
  renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
  renderingInfo.renderArea.offset = {0, 0};
  renderingInfo.renderArea.extent = renderTarget->extent();
#if defined(MAGMA_WITH_EDITOR)
  renderingInfo.colorAttachmentCount = 2;
#else
  renderingInfo.colorAttachmentCount = 1;
#endif
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

void OffscreenRenderer::record() {
  pipeline->bind(FrameInfo::commandBuffer);
  auto set = renderContext->getDescriptorSet(LayoutKey::Camera, currentImageIndex());

  if (!set.has_value())
    throw runtime_error("No descriptor set found for OffscreenRenderer!");

  uint32_t offset = rendererId * renderContext->cameraSliceSize();

  vkCmdBindDescriptorSets(FrameInfo::commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS, getPipelineLayout(),
                          0, 1, &set.value(), 1, 
                          &offset);
}

void OffscreenRenderer::end() {
  if (FrameInfo::commandBuffer == VK_NULL_HANDLE)
    throw runtime_error("No command buffer found in FrameInfo!");
  if (FrameInfo::frameIndex < 0 ||
      FrameInfo::frameIndex >= SwapChain::MAX_FRAMES_IN_FLIGHT)
    throw runtime_error("Invalid frame index in FrameInfo!");

  vkCmdEndRendering(FrameInfo::commandBuffer);

#if defined(MAGMA_WITH_EDITOR)
  const uint32_t idx = currentImageIndex();

  // Transition scene color to SHADER_READ_ONLY for ImGui sampling
  {
    VkImage sceneColor = renderTarget->getColorImage(idx);
    Device::transitionImageLayout(
        sceneColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
    sceneColorLayouts[idx] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  // Transition ID image to SHADER_READ_ONLY (used for readback barriers)
  {
    VkImage idImage =
        static_cast<OffscreenTarget *>(renderTarget.get())->getIdImage();
    Device::transitionImageLayout(
        idImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);
    idColorLayouts[idx] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  }

  // Safe point: service any pending pick request now (no render pass active)
  servicePendingPick();
#else
  const uint32_t idx = currentImageIndex();

  // Transition scene color to PRESENT_SRC for swapchain presentation
  {
    VkImage sceneColor = renderTarget->getColorImage(idx);
    Device::transitionImageLayout(
        sceneColor, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0, VK_IMAGE_ASPECT_COLOR_BIT);
    sceneColorLayouts[idx] = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  }
#endif
}

// --- Resize ---
#if defined(MAGMA_WITH_EDITOR)
void OffscreenRenderer::resize(VkExtent2D newExtent) {
  Device::waitIdle();

  for (auto texture : textures)
    ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)texture);

  renderTarget->resize(newExtent);
  createPipeline(renderTarget.get());

  createOffscreenTextures();

  sceneColorLayouts.assign(renderTarget->imageCount(),
                           VK_IMAGE_LAYOUT_UNDEFINED);
  idColorLayouts.assign(renderTarget->imageCount(), VK_IMAGE_LAYOUT_UNDEFINED);
}
#else
void OffscreenRenderer::resize(VkExtent2D newExtent, VkSwapchainKHR swapChain) {
  Device::waitIdle();

  renderTarget->resize(newExtent, swapChain);
  createPipeline(renderTarget.get());

  sceneColorLayouts.assign(renderTarget->imageCount(),
                           VK_IMAGE_LAYOUT_UNDEFINED);
}
#endif

#if defined(MAGMA_WITH_EDITOR)
GameObject *OffscreenRenderer::pickAtPixel(uint32_t x, uint32_t y) {
  VkExtent2D extent = renderTarget->extent();
  if (x >= extent.width || y >= extent.height)
    return nullptr;

  Buffer stagingBuffer(sizeof(uint32_t), 1, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  stagingBuffer.map();

  VkCommandBuffer commandBuffer = Device::get().beginSingleTimeCommands();
  VkImage idImage = renderTarget->getIdImage();

  // Transition ID image for readback (from shader read-only to transfer src)
  VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = idImage;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {static_cast<int32_t>(x), static_cast<int32_t>(y), 0};
  region.imageExtent = {1, 1, 1};

  Device::get().copyImageToBuffer(commandBuffer, stagingBuffer.getBuffer(),
                                  idImage, region);

  // Transition back to shader read-only
  VkImageMemoryBarrier barrier2{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier2.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  barrier2.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier2.image = idImage;
  barrier2.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier2.subresourceRange.baseMipLevel = 0;
  barrier2.subresourceRange.levelCount = 1;
  barrier2.subresourceRange.baseArrayLayer = 0;
  barrier2.subresourceRange.layerCount = 1;
  barrier2.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
  barrier2.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier2);

  Device::get().endSingleTimeCommands(commandBuffer);

  uint32_t objectId = 0;
  void *data = stagingBuffer.mappedData();
  if (data)
    memcpy(&objectId, data, sizeof(uint32_t));

  if (objectId == 0) // No object picked
    return nullptr;

  if (Scene::current())
    return Scene::current()->findGameObjectById(
        static_cast<GameObject::id_t>(objectId));

  return nullptr;
}

void OffscreenRenderer::servicePendingPick() {
  if (!pendingPick.hasRequest)
    return;

  // Execute GPU readback now (no dynamic rendering active on FrameInfo CB)
  GameObject *picked = pickAtPixel(pendingPick.x, pendingPick.y);
  pendingPick.result = picked;
  pendingPick.hasRequest = false;
}
#endif

} // namespace Magma
