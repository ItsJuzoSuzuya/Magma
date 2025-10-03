#include "offscreen_renderer.hpp"
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
VkImage &OffscreenRenderer::getSceneImage(uint32_t frameIndex) const {
  return renderTarget->getColorImage(frameIndex);
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
void OffscreenRenderer::begin(VkCommandBuffer commandBuffer,
                              uint32_t frameIndex) {
  array<VkClearValue, 2> clearValues{};
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

void OffscreenRenderer::record(VkCommandBuffer cmd) { pipeline->bind(cmd); }

void OffscreenRenderer::end(VkCommandBuffer cmd) { vkCmdEndRenderPass(cmd); }

} // namespace Magma
