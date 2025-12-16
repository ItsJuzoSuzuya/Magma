#include "render_context.hpp"
#include "../../core/swapchain.hpp"
#include "../components/camera.hpp"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <ostream>
#include <print>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

// ============== SlicedResource Implementation ==============

SlicedResource::SlicedResource(VkDeviceSize elementSize,
                               VkBufferUsageFlags usage,
                               uint32_t framesInFlight)
    : elementSize{elementSize}, usage{usage}, framesInFlight{framesInFlight} {}

void SlicedResource::ensureCapacity(uint32_t rendererCount) {
  if (rendererCount <= currentCapacity)
    return;

  // Grow by doubling or to requested size, whichever is larger
  uint32_t newCapacity = max(rendererCount, currentCapacity * 2);
  if (newCapacity == 0)
    newCapacity = 1;

  reallocate(newCapacity);
}

void SlicedResource::update(uint32_t frameIndex, uint32_t sliceIndex,
                            const void *data, VkDeviceSize size) {
  if (!buffer)
    return;

  VkDeviceSize offset =
      frameIndex * (elementSize * currentCapacity) + sliceIndex * elementSize;

  buffer->writeToBuffer(const_cast<void *>(data), size, offset);
  buffer->flush(size, offset);
}

void SlicedResource::reallocate(uint32_t newCapacity) {
  buffer = make_unique<Buffer>(elementSize, newCapacity * framesInFlight, usage,
                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  buffer->map();
  currentCapacity = newCapacity;
}

// ============== RenderContext Implementation ==============

uint32_t RenderContext::registerRenderer() {
  uint32_t index = registeredRendererCount++;
  println("Registered renderer with index {}", index);

  // Ensure resources have capacity for this renderer (if they already
  // exist)
  if (cameraResource) {
    uint32_t oldCapacity = cameraResource->getCapacity();
    cameraResource->ensureCapacity(registeredRendererCount);
    if (cameraResource->getCapacity() > oldCapacity)
      setsNeedRebuild[LayoutKey::Camera] = true;
  }

  if (pointLightResource) {
    uint32_t oldCapacity = pointLightResource->getCapacity();
    pointLightResource->ensureCapacity(registeredRendererCount);
    if (pointLightResource->getCapacity() > oldCapacity)
      setsNeedRebuild[LayoutKey::PointLight] = true;
  }

  return index;
}

VkDescriptorSetLayout RenderContext::getLayout(LayoutKey key) {
  ensureLayout(key);
  return descriptorSetLayouts[key]->getDescriptorSetLayout();
}

void RenderContext::createDescriptorSets(LayoutKey key) {
  println("Creating descriptor sets for layout key {}", static_cast<int>(key));
  rebuildDescriptorSetsIfNeeded(key);

  for (uint32_t frameIndex = 0; frameIndex < SwapChain::MAX_FRAMES_IN_FLIGHT;
       frameIndex++) {
    DescriptorKey dKey{key, frameIndex};
    if (setCache.find(dKey) != setCache.end())
      continue;

    writeDescriptorSet(key, frameIndex);
  }
}

std::optional<VkDescriptorSet>
RenderContext::getDescriptorSet(LayoutKey key, uint32_t frameIndex) {
  rebuildDescriptorSetsIfNeeded(key);

  DescriptorKey dKey{key, frameIndex};
  auto it = setCache.find(dKey);
  if (it == setCache.end())
    return nullopt;

  return it->second;
}

uint32_t RenderContext::cameraSliceSize() const {
  return cameraResource ? static_cast<uint32_t>(cameraResource->getSliceSize())
                        : sizeof(CameraUBO);
}

void RenderContext::updateCameraSlice(uint32_t frameIndex, uint32_t sliceIndex,
                                      const void *data, VkDeviceSize size) {
  ensureCameraResource();

  uint32_t oldCapacity = cameraResource->getCapacity();
  cameraResource->ensureCapacity(sliceIndex + 1);
  if (cameraResource->getCapacity() > oldCapacity)
    setsNeedRebuild[LayoutKey::Camera] = true;

  cameraResource->update(frameIndex, sliceIndex, data, size);
}

uint32_t RenderContext::pointLightSliceSize() const {
  return pointLightResource
             ? static_cast<uint32_t>(pointLightResource->getSliceSize())
             : sizeof(PointLightSSBO);
}

void RenderContext::updatePointLightSlice(uint32_t frameIndex,
                                          uint32_t sliceIndex, const void *data,
                                          VkDeviceSize size) {
  ensurePointLightResource();

  uint32_t oldCapacity = pointLightResource->getCapacity();
  pointLightResource->ensureCapacity(sliceIndex + 1);
  if (pointLightResource->getCapacity() > oldCapacity)
    setsNeedRebuild[LayoutKey::PointLight] = true;

  pointLightResource->update(frameIndex, sliceIndex, data, size);
}

// ============== Private Helpers ==============

void RenderContext::ensureDescriptorPool() {
  if (descriptorPool)
    return;

  descriptorPool =
      DescriptorPool::Builder()
          .setMaxSets(512)
          .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 256)
          .addPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 256)
          .setPoolFlags(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT)
          .build();
}

void RenderContext::ensureLayout(LayoutKey key) {
  if (descriptorSetLayouts.find(key) != descriptorSetLayouts.end())
    return;

  switch (key) {
  case LayoutKey::Camera:
    descriptorSetLayouts[key] =
        DescriptorSetLayout::Builder()
            .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
                        VK_SHADER_STAGE_VERTEX_BIT)
            .build();
    break;

  case LayoutKey::PointLight:
    descriptorSetLayouts[key] =
        DescriptorSetLayout::Builder()
            .addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                        VK_SHADER_STAGE_FRAGMENT_BIT)
            .build();
    break;
  }
}

void RenderContext::ensureCameraResource() {
  if (cameraResource)
    return;

  println("Creating camera resource");
  cameraResource = make_unique<SlicedResource>(
      sizeof(CameraUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      SwapChain::MAX_FRAMES_IN_FLIGHT);

  cameraResource->ensureCapacity(max(1u, registeredRendererCount));
  setsNeedRebuild[LayoutKey::Camera] = true;
}

void RenderContext::ensurePointLightResource() {
  if (pointLightResource)
    return;

  println("Creating point light resource");
  pointLightResource = make_unique<SlicedResource>(
      sizeof(PointLightSSBO), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      SwapChain::MAX_FRAMES_IN_FLIGHT);

  pointLightResource->ensureCapacity(max(1u, registeredRendererCount));
  setsNeedRebuild[LayoutKey::PointLight] = true;
}

void RenderContext::writeDescriptorSet(LayoutKey key, uint32_t frameIndex) {
  ensureDescriptorPool();
  ensureLayout(key);

  DescriptorKey dsKey{key, frameIndex};

  VkDescriptorSet set{};
  DescriptorWriter writer(*descriptorSetLayouts[key], *descriptorPool);

  if (key == LayoutKey::Camera) {
    ensureCameraResource();
    VkDescriptorBufferInfo info = {};
    info.buffer = cameraResource->getBuffer();
    info.offset = 0;
    info.range = cameraResource->getSliceSize();
    writer.writeBuffer(0, &info, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);
  } else if (key == LayoutKey::PointLight) {
    ensurePointLightResource();
    VkDescriptorBufferInfo info = {};
    info.buffer = pointLightResource->getBuffer();
    info.offset = 0;
    info.range = pointLightResource->getSliceSize();
    writer.writeBuffer(0, &info, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
  } else {
    throw runtime_error("Unknown layout key");
  }

  if (!writer.build(set))
    throw runtime_error("Failed to build descriptor set");

  setCache[dsKey] = set;
}

void RenderContext::rebuildDescriptorSetsIfNeeded(LayoutKey key) {
  auto it = setsNeedRebuild.find(key);
  if (it == setsNeedRebuild.end() || !it->second)
    return;

  // Clear old sets for this key
  for (uint32_t frameIndex = 0; frameIndex < SwapChain::MAX_FRAMES_IN_FLIGHT;
       frameIndex++) {
    DescriptorKey dKey{key, frameIndex};
    setCache.erase(dKey);
  }

  // Rebuild
  for (uint32_t frameIndex = 0; frameIndex < SwapChain::MAX_FRAMES_IN_FLIGHT;
       frameIndex++) {
    writeDescriptorSet(key, frameIndex);
  }

  setsNeedRebuild[key] = false;
}

} // namespace Magma
