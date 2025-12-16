#pragma once

#include "../../core/buffer.hpp"
#include "../../core/descriptors.hpp"
#include "../components/point_light.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

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
                 uint32_t framesInFlight);
  ~SlicedResource() = default;

  void ensureCapacity(uint32_t rendererCount);
  void update(uint32_t frameIndex, uint32_t sliceIndex, const void *data,
              VkDeviceSize size);

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

  void reallocate(uint32_t newCapacity);
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

  // Delete copy/move
  RenderContext(const RenderContext &) = delete;
  RenderContext &operator=(const RenderContext &) = delete;

  /**
   * Register a renderer that needs resources.
   * @return The renderer's slice index for accessing per-renderer data.
   */
  uint32_t registerRenderer();

  /**
   * Get the number of registered renderers.
   */
  uint32_t getRendererCount() const { return registeredRendererCount; }

  // Layout access (created lazily)
  VkDescriptorSetLayout getLayout(LayoutKey key);

  // Descriptor set access (created lazily)
  void createDescriptorSets(LayoutKey key);
  std::optional<VkDescriptorSet> getDescriptorSet(LayoutKey key,
                                                  uint32_t frameIndex);

  // Camera resource access
  uint32_t cameraSliceSize() const;
  void updateCameraSlice(uint32_t frameIndex, uint32_t sliceIndex,
                         const void *data, VkDeviceSize size);

  // Point light resource access
  uint32_t pointLightSliceSize() const;
  void updatePointLightSlice(uint32_t frameIndex, uint32_t sliceIndex,
                             const void *data, VkDeviceSize size);

private:
  // Descriptor pool (lazily created)
  std::unique_ptr<DescriptorPool> descriptorPool;
  void ensureDescriptorPool();

  // Layouts (lazily created)
  std::unordered_map<LayoutKey, std::unique_ptr<DescriptorSetLayout>>
      descriptorSetLayouts;
  void ensureLayout(LayoutKey key);

  // Resources (lazily created)
  std::unique_ptr<SlicedResource> cameraResource;
  std::unique_ptr<SlicedResource> pointLightResource;
  void ensureCameraResource();
  void ensurePointLightResource();

  // Descriptor set cache
  struct DescriptorKey {
    LayoutKey key;
    uint32_t frameIndex;

    bool operator==(const DescriptorKey &other) const {
      return key == other.key && frameIndex == other.frameIndex;
    }
  };
  struct DescriptorKeyHash {
    size_t operator()(const DescriptorKey &k) const {
      return static_cast<size_t>(k.key) * 73856093u ^ k.frameIndex * 19349663u;
    }
  };
  std::unordered_map<DescriptorKey, VkDescriptorSet, DescriptorKeyHash>
      setCache;

  // Tracks if descriptor sets need rebuilding after resource reallocation
  std::unordered_map<LayoutKey, bool> setsNeedRebuild;

  uint32_t registeredRendererCount = 0;

  void writeDescriptorSet(LayoutKey key, uint32_t frameIndex);
  void rebuildDescriptorSetsIfNeeded(LayoutKey key);
};

} // namespace Magma
