#include "offscreen_renderer.hpp"
#include "../../core/frame_info.hpp"
#include "../../core/render_target_info.hpp"
#include <array>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

// Constructor
OffscreenRenderer::OffscreenRenderer(Device &device, RenderTargetInfo &info,
                                     VkDescriptorSetLayout descriptorSetLayout)
    : Renderer(device, descriptorSetLayout) {
  renderTarget = make_unique<RenderTarget>(device, info);
  createPipeline();
}

// Destructor
OffscreenRenderer::~OffscreenRenderer() {
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

  array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.f, 0.f, 0.f, 1.f}};
  clearValues[1].depthStencil = {1.0f, 0};

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

void OffscreenRenderer::record() { pipeline->bind(FrameInfo::commandBuffer); }

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

// Resize
void OffscreenRenderer::resize(VkExtent2D newExtent) {
  for (auto texture : textures)
    ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)texture);

  renderTarget->resize(newExtent);
  createPipeline();
  createOffscreenTextures();
}

} // namespace Magma
