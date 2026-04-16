#include "render_context.hpp"
#include "core/swapchain.hpp"
#include "engine/components/point_light.hpp"
#include <print>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Magma {

RenderContext::~RenderContext(){
  descriptorPool = nullptr;
  layouts.clear();

  for (auto &buffer: pointLightBuffers)
    buffer->cleanUp();
}

VkDescriptorSetLayout RenderContext::getLayout(LayoutKey key) {
  ensureLayout(key);
  return layouts[key]->getDescriptorSetLayout();
}

VkDescriptorSet RenderContext::getDescriptorSet(LayoutKey key, uint32_t frameIndex) {
  if (key == LayoutKey::PointLight)
    return pointLightSets[frameIndex];
  throw std::runtime_error("RenderContext: unknown LayoutKey");
}

void RenderContext::updatePointLights(uint32_t frameIndex, const void *data, VkDeviceSize size) {
  pointLightBuffers[frameIndex]->writeToBuffer(const_cast<void *>(data), size, 0);
  pointLightBuffers[frameIndex]->flush(size, 0);
}

// -----------------------------------------------------------------------------
// Private
// -----------------------------------------------------------------------------

void RenderContext::ensureDescriptorPool() {
  if (descriptorPool)
    return;
  descriptorPool = DescriptorPool::Builder()
      .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
      .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
      .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
      .build();
}

void RenderContext::ensureLayout(LayoutKey key) {
  if (layouts.count(key))
    return;

  switch (key) {
  case LayoutKey::PointLight:
    layouts[key] = DescriptorSetLayout::Builder()
        .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .build();
    initPointLightBuffers();
    break;
  }
}

void RenderContext::initPointLightBuffers() {
  if (pointLightInitialized)
    return;

  ensureDescriptorPool();

  for (uint32_t i = 0; i < SwapChain::MAX_FRAMES_IN_FLIGHT; i++) {
    pointLightBuffers[i] = std::make_unique<Buffer>(
        sizeof(PointLightSSBO), 1,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    pointLightBuffers[i]->map();

    VkDescriptorBufferInfo info{};
    info.buffer = pointLightBuffers[i]->getBuffer();
    info.offset = 0;
    info.range  = sizeof(PointLightSSBO);

    DescriptorWriter(*layouts[LayoutKey::PointLight], *descriptorPool)
        .writeBuffer(0, &info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
        .build(pointLightSets[i]);
  }

  pointLightInitialized = true;
}

} // namespace Magma
