#include "render_context.hpp"
#include "../../core/swapchain.hpp"
#include "../components/camera.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

RenderContext::RenderContext(uint32_t rendererCount) {
  createDescriptorPool();
  createDescriptorSetLayouts();
  createPerFrameBuffers(rendererCount);
}

// --- Public --- //
VkDescriptorSetLayout RenderContext::getLayout(LayoutKey key) const {
  auto it = descriptorSetLayouts.find(key);
  if (it == descriptorSetLayouts.end())
    throw runtime_error("DescriptorSetLayout not found for given key!");

  return it->second->getDescriptorSetLayout();
}

void RenderContext::createDescriptorSets(LayoutKey key) {
  for (uint32_t frameIndex = 0; frameIndex < SwapChain::MAX_FRAMES_IN_FLIGHT;
       frameIndex++) {
    DescriptorKey dKey{key, frameIndex};
    if (setCache.find(dKey) != setCache.end())
      return;

    VkDescriptorSet descriptorSet{};
    writeDescriptorSet(key, frameIndex);
    descriptorSet = setCache[dKey];
  }
}
std::optional<VkDescriptorSet>
RenderContext::getDescriptorSet(LayoutKey key, uint32_t frameIndex) {
  DescriptorKey dKey{key, frameIndex};
  auto it = setCache.find(dKey);
  if (it == setCache.end())
    return nullopt; // not found

  return it->second;
}

void RenderContext::updateCameraSlice(uint32_t frameIndex, uint32_t sliceIndex,
                                      const void *data, VkDeviceSize size) {
  VkDeviceSize offset =
      frameIndex * (cameraPF.sliceSize * cameraPF.sliceCount) +
      sliceIndex * cameraPF.sliceSize;
  cameraPF.cameraBuffer->writeToBuffer(const_cast<void *>(data), size, offset);
  cameraPF.cameraBuffer->flush(size, offset);
}

void RenderContext::updatePointLightSlice(uint32_t frameIndex,
                                          uint32_t sliceIndex, const void *data,
                                          VkDeviceSize size) {
  VkDeviceSize offset =
      frameIndex * (pointLightPF.sliceSize * pointLightPF.sliceCount) +
      sliceIndex * pointLightPF.sliceSize;
  pointLightPF.pointLightBuffer->writeToBuffer(const_cast<void *>(data), size,
                                               offset);
  pointLightPF.pointLightBuffer->flush(size, offset);
}

// --- Private --- //

void RenderContext::createDescriptorPool() {
  descriptorPool =
      DescriptorPool::Builder()
          .setMaxSets(512) // enough for all frames and uses
          .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 256)
          .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 256)
          .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
          .build();
}

void RenderContext::createDescriptorSetLayouts() {
  auto cameraLayout =
      DescriptorSetLayout::Builder()
          .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                      VK_SHADER_STAGE_VERTEX_BIT)
          .build();
  descriptorSetLayouts[LayoutKey::Camera] = std::move(cameraLayout);

  auto pointLightLayout = DescriptorSetLayout::Builder()
                              .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                          VK_SHADER_STAGE_FRAGMENT_BIT)
                              .build();
  descriptorSetLayouts[LayoutKey::PointLight] = std::move(pointLightLayout);
}

void RenderContext::createPerFrameBuffers(uint32_t sliceCount) {
  cameraPF.sliceSize = sizeof(CameraUBO);
  cameraPF.sliceCount = sliceCount;
  cameraPF.cameraBuffer = make_unique<Buffer>(
      cameraPF.sliceSize, cameraPF.sliceCount * SwapChain::MAX_FRAMES_IN_FLIGHT,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  cameraPF.cameraBuffer->map();

  pointLightPF.sliceSize = sizeof(PointLightSSBO);
  pointLightPF.sliceCount = sliceCount;
  pointLightPF.pointLightBuffer = make_unique<Buffer>(
      pointLightPF.sliceSize,
      pointLightPF.sliceCount * SwapChain::MAX_FRAMES_IN_FLIGHT,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  pointLightPF.pointLightBuffer->map();
}

void RenderContext::writeDescriptorSet(LayoutKey key, uint32_t frameIndex) {
  DescriptorKey dsKey{key, frameIndex};
  if (setCache.find(dsKey) != setCache.end())
    return;

  VkDescriptorSet set{};
  DescriptorWriter writer(*descriptorSetLayouts[key], *descriptorPool);

  if (key == LayoutKey::Camera) {
    VkDescriptorBufferInfo info = {};
    info.buffer = cameraPF.cameraBuffer->getBuffer();
    info.offset = 0;
    info.range = cameraPF.sliceSize;
    writer.writeBuffer(0, &info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
  } else if (key == LayoutKey::PointLight) {
    VkDescriptorBufferInfo info = {};
    info.buffer = pointLightPF.pointLightBuffer->getBuffer();
    info.offset = 0;
    info.range = pointLightPF.sliceSize;
    writer.writeBuffer(0, &info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
  } else {
    throw std::runtime_error("Unknown layout key");
  }

  if (!writer.build(set))
    throw std::runtime_error("Failed to build descriptor set");
  setCache[dsKey] = set;
}

} // namespace Magma
