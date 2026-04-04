module;
#include <vulkan/vulkan_core.h>

module render:render_context;
import core;

namespace Magma {

enum class LayoutKey {
  Camera = 0,
  PointLight = 1,
};

/**
 * Manages a sliced buffer for per-renderer, per-frame data.
 * Lazily allocated and auto-growing.
 */
class SlicedResource {
public:
  SlicedResource(VkDeviceSize elementSize, VkBufferUsageFlags usage, 
      uint32_t framesInFlight
  ): elementSize{elementSize}, usage{usage}, framesInFlight{framesInFlight} {}
  ~SlicedResource() = default;

  void ensureCapacity(uint32_t rendererCount) {
    if (rendererCount <= currentCapacity)
      return;

    // Grow by doubling or to requested size, whichever is larger
    uint32_t newCapacity = std::max(rendererCount, currentCapacity * 2);
    if (newCapacity == 0)
      newCapacity = 1;

    reallocate(newCapacity);
  }

  void update(uint32_t frameIndex, uint32_t sliceIndex,
                              const void *data, VkDeviceSize size) {
    if (!buffer)
      return;

    VkDeviceSize offset =
        frameIndex * (elementSize * currentCapacity) + sliceIndex * elementSize;

    buffer->writeToBuffer(const_cast<void *>(data), size, offset);
    buffer->flush(size, offset);
  }

  VkBuffer getBuffer() const {
    return buffer ? buffer->getBuffer() : VK_NULL_HANDLE;
  }
  VkDeviceSize getSliceSize() const { return elementSize; }
  uint32_t getCapacity() const { return currentCapacity; }
  bool isAllocated() const { return buffer != nullptr; }

private:
  VkDeviceSize elementSize;
  VkBufferUsageFlags usage;
  uint32_t framesInFlight;
  uint32_t currentCapacity = 0;
  std::unique_ptr<Buffer> buffer;

  void reallocate(uint32_t newCapacity) {
    buffer = std::make_unique<Buffer>(elementSize, newCapacity * framesInFlight, usage,
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    buffer->map();
    currentCapacity = newCapacity;
  }
};

/**
 * Central render resource manager with lazy allocation.
 *
 * Resources are only created when first requested:
 * - No lights in scene?  PointLight buffers never allocated.
 * - Renderers self-register and get a slice index back.
 */
class RenderContext {
public:
  RenderContext() = default;
  ~RenderContext() = default;

  RenderContext(const RenderContext &) = delete;
  RenderContext &operator=(const RenderContext &) = delete;

  /**
   * Register a renderer that needs resources.
   * @return The renderer's slice index for accessing per-renderer data.
   */
  uint32_t registerRenderer() {
    uint32_t index = registeredRendererCount++;

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
  uint32_t getRendererCount() const { return registeredRendererCount; }

  VkDescriptorSetLayout getLayout(LayoutKey key) {
    ensureLayout(key);
    return descriptorSetLayouts[key]->getDescriptorSetLayout();
  }

  // Descriptor Sets
  void createDescriptorSets(LayoutKey key) {
    rebuildDescriptorSetsIfNeeded(key);

    for (uint32_t frameIndex = 0; frameIndex < SwapChain::MAX_FRAMES_IN_FLIGHT;
         frameIndex++) {
      DescriptorSetKey dKey{key, frameIndex};
      if (setCache.find(dKey) != setCache.end())
        continue;

      writeDescriptorSet(key, frameIndex);
    }
  }
  std::optional<VkDescriptorSet> getDescriptorSet(LayoutKey key, uint32_t frameIndex) {
    rebuildDescriptorSetsIfNeeded(key);

    DescriptorSetKey dKey{key, frameIndex};
    auto it = setCache.find(dKey);
    if (it == setCache.end())
      return std::nullopt;

    return it->second;
  }


  // Camera Slice
  uint32_t cameraSliceSize() const {
    return cameraResource ? static_cast<uint32_t>(cameraResource->getSliceSize())
                          : sizeof(CameraUBO);
  }
  void updateCameraSlice(uint32_t frameIndex, uint32_t sliceIndex,
                         const void *data, VkDeviceSize size) {
    ensureCameraResource();

    uint32_t oldCapacity = cameraResource->getCapacity();
    cameraResource->ensureCapacity(sliceIndex + 1);
    if (cameraResource->getCapacity() > oldCapacity)
      setsNeedRebuild[LayoutKey::Camera] = true;

    cameraResource->update(frameIndex, sliceIndex, data, size);
  }

  uint32_t pointLightSliceSize() const {
    return pointLightResource
               ? static_cast<uint32_t>(pointLightResource->getSliceSize())
               : sizeof(PointLightSSBO);
  }

  void updatePointLightSlice(uint32_t frameIndex,
                             uint32_t sliceIndex, const void *data,
                             VkDeviceSize size) {
    ensurePointLightResource();

    uint32_t oldCapacity = pointLightResource->getCapacity();
    pointLightResource->ensureCapacity(sliceIndex + 1);
    if (pointLightResource->getCapacity() > oldCapacity)
      setsNeedRebuild[LayoutKey::PointLight] = true;

    pointLightResource->update(frameIndex, sliceIndex, data, size);
  }

private:
  uint32_t registeredRendererCount = 0;

  std::unique_ptr<DescriptorPool> descriptorPool;
  void ensureDescriptorPool() {
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

  std::unordered_map<LayoutKey, std::unique_ptr<DescriptorSetLayout>>
      descriptorSetLayouts;
  void ensureLayout(LayoutKey key) {
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

  std::unique_ptr<SlicedResource> cameraResource;
  std::unique_ptr<SlicedResource> pointLightResource;
  void RenderContext::ensureCameraResource() {
    if (cameraResource)
      return;

    cameraResource = std::make_unique<SlicedResource>(
        sizeof(CameraUBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        SwapChain::MAX_FRAMES_IN_FLIGHT);

    cameraResource->ensureCapacity(std::max(1u, registeredRendererCount));
    setsNeedRebuild[LayoutKey::Camera] = true;
  }

  void RenderContext::ensurePointLightResource() {
    if (pointLightResource)
      return;

    pointLightResource = std::make_unique<SlicedResource>(
        sizeof(PointLightSSBO), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        SwapChain::MAX_FRAMES_IN_FLIGHT);

    pointLightResource->ensureCapacity(std::max(1u, registeredRendererCount));
    setsNeedRebuild[LayoutKey::PointLight] = true;
  }

  // Descriptor set cache
  struct DescriptorSetKey {
    LayoutKey key;
    uint32_t frameIndex;

    bool operator==(const DescriptorSetKey &other) const {
      return key == other.key && frameIndex == other.frameIndex;
    }
  };
  struct DescriptorSetKeyHash {
    size_t operator()(const DescriptorSetKey &k) const {
      return static_cast<size_t>(k.key) * 73856093u ^ k.frameIndex * 19349663u;
    }
  };
  std::unordered_map<DescriptorSetKey, VkDescriptorSet, DescriptorSetKeyHash>
      setCache;
  void writeDescriptorSet(LayoutKey key, uint32_t frameIndex) {
    ensureDescriptorPool();
    ensureLayout(key);

    DescriptorSetKey dsKey{key, frameIndex};

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
      throw std::runtime_error("Unknown layout key");
    }

    if (!writer.build(set))
      throw std::runtime_error("Failed to build descriptor set");

    setCache[dsKey] = set;
  }

  // Tracks if descriptor sets need rebuilding after resource reallocation
  std::unordered_map<LayoutKey, bool> setsNeedRebuild;
  void rebuildDescriptorSetsIfNeeded(LayoutKey key) {
    auto it = setsNeedRebuild.find(key);
    if (it == setsNeedRebuild.end() || !it->second)
      return;

    for (uint32_t frameIndex = 0; frameIndex < SwapChain::MAX_FRAMES_IN_FLIGHT;
         frameIndex++) {
      DescriptorSetKey dKey{key, frameIndex};
      setCache.erase(dKey);
    }

    for (uint32_t frameIndex = 0; frameIndex < SwapChain::MAX_FRAMES_IN_FLIGHT;
         frameIndex++) {
      writeDescriptorSet(key, frameIndex);
    }

    setsNeedRebuild[key] = false;
  }

};

} // namespace Magma
