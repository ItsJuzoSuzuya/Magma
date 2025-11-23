#include "offscreen_renderer.hpp"
#include "../../core/frame_info.hpp"
#include "../../core/device.hpp"
#include "../../core/render_target_info.hpp"
#include "../scene.hpp"
#include "imgui_impl_vulkan.h"
#include "offscreen_target.hpp"
#include "../components/camera.hpp"
#include <array>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

// Constructor
OffscreenRenderer::OffscreenRenderer(RenderTargetInfo &info)
    : Renderer() {
  cameraBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (uint32_t i = 0; i < cameraBuffers.size(); ++i){
    cameraBuffers[i] = make_unique<Buffer>(
        sizeof(CameraUBO), 1,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT );
    cameraBuffers[i]->map();
  }

  createDescriptorPool();
  createDescriptorSetLayout();
  Renderer::init(descriptorSetLayout->getDescriptorSetLayout());
  createDescriptorSets();

  renderTarget = make_unique<OffscreenTarget>(info);
  createPipeline(renderTarget.get(), "src/shaders/shader.vert.spv",
                 "src/shaders/shader.frag.spv");
}

// Destructor
OffscreenRenderer::~OffscreenRenderer() {
  vkDeviceWaitIdle(Device::get().device());
  for (auto texture : textures)
    ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)texture);
}

// Getters
VkImage &OffscreenRenderer::getSceneImage() const {
  return renderTarget->getColorImage(FrameInfo::frameIndex);
}
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

// Rendering helpers
void OffscreenRenderer::begin() {
  if (FrameInfo::commandBuffer == VK_NULL_HANDLE)
    throw runtime_error("No command buffer found in FrameInfo!");
  if (FrameInfo::frameIndex < 0 ||
      FrameInfo::frameIndex >= SwapChain::MAX_FRAMES_IN_FLIGHT)
    throw runtime_error("Invalid frame index in FrameInfo!");

  array<VkClearValue, 3> clearValues{};
  clearValues[0].color = {{0.f, 0.f, 0.f, 1.f}};
  clearValues[1].color = {};
  clearValues[1].color.uint32[0] = 0;
  clearValues[1].color.uint32[1] = 0;
  clearValues[1].color.uint32[2] = 0;
  clearValues[1].color.uint32[3] = 0;
  clearValues[2].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo beginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  beginInfo.renderPass = renderTarget->getRenderPass();
  beginInfo.framebuffer = renderTarget->getFrameBuffer(FrameInfo::frameIndex);
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

void OffscreenRenderer::record() { 
  pipeline->bind(FrameInfo::commandBuffer); 
  vkCmdBindDescriptorSets(FrameInfo::commandBuffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          getPipelineLayout(), 0, 1,
                          &descriptorSets[FrameInfo::frameIndex], 0, nullptr);
}

void OffscreenRenderer::end() {
  if (FrameInfo::commandBuffer == VK_NULL_HANDLE)
    throw runtime_error("No command buffer found in FrameInfo!");
  if (FrameInfo::frameIndex < 0 ||
      FrameInfo::frameIndex >= SwapChain::MAX_FRAMES_IN_FLIGHT)
    throw runtime_error("Invalid frame index in FrameInfo!");

  vkCmdEndRenderPass(FrameInfo::commandBuffer);

  VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = getSceneImage();
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  vkCmdPipelineBarrier(FrameInfo::commandBuffer,
                       VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);
}

// --- Resize ---
void OffscreenRenderer::resize(VkExtent2D newExtent) {
  Device::waitIdle();
  for (auto texture : textures)
    ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)texture);

  renderTarget->resize(newExtent);
  createPipeline(renderTarget.get());
  createOffscreenTextures();
}

GameObject *OffscreenRenderer::pickAtPixel(uint32_t x, uint32_t y) {
  VkExtent2D extent = renderTarget->extent();
  if (x >= extent.width || y >= extent.height)
    return nullptr;

  Buffer stagingBuffer(
      sizeof(uint32_t), 1,
      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  stagingBuffer.map();

  VkCommandBuffer commandBuffer = Device::get().beginSingleTimeCommands();
  VkImage idImage = renderTarget->getIdImage();

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

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
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

  Device::get().copyImageToBuffer(
      commandBuffer, stagingBuffer.getBuffer(), idImage, region);

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

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
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
    return Scene::current()->findGameObjectById(static_cast<GameObject::id_t>(objectId));

  return nullptr;
}

// --- Private --- //
// --- Desciptors ---
void OffscreenRenderer::createDescriptorPool() {
  descriptorPool =
      DescriptorPool::Builder()
          .setMaxSets(2 * SwapChain::MAX_FRAMES_IN_FLIGHT)
          .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                       2 * SwapChain::MAX_FRAMES_IN_FLIGHT)
          .build();
}

void OffscreenRenderer::createDescriptorSetLayout() {
  descriptorSetLayout = DescriptorSetLayout::Builder()
    .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                VK_SHADER_STAGE_VERTEX_BIT)
    .build();
}

void OffscreenRenderer::createDescriptorSets() {
  descriptorSets.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);
  for (size_t i = 0; i < static_cast<uint32_t>(descriptorSets.size()); i++) {
    VkDescriptorBufferInfo cameraBufferInfo =
        cameraBuffers[i]->descriptorInfo();
    DescriptorWriter(*descriptorSetLayout, *descriptorPool)
        .writeBuffer(0, &cameraBufferInfo)
        .build(descriptorSets[i]);
  }
}

} // namespace Magma
