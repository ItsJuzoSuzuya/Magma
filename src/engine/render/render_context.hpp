#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vulkan/vulkan_core.h>
#include "../core/buffer.hpp"
#include "../core/descriptors.hpp"

namespace Magma {

enum LayoutKey {
  Camera = 0,
  PointLight = 1,
};

/**
  * Per-frame camera data
  * @note One buffer per frame, with multiple slices for multiple cameras
  */
struct PerFrameCamera {
  std::unique_ptr<Buffer> cameraBuffer;
  VkDeviceSize sliceSize = 0;
  uint32_t sliceCount = 0;
};

class RenderContext {
public:
  RenderContext(uint32_t rendererCount);
  ~RenderContext() = default;

  VkDescriptorSetLayout getLayout(LayoutKey key) const;
  PerFrameCamera &cameras() { return cameraPF; }

  // Set Management
  void createDescriptorSets(LayoutKey key);
  std::optional<VkDescriptorSet> getDescriptorSet(LayoutKey key, uint32_t frameIndex);

  // Update 
  void updateCameraSlice(uint32_t frameIndex,uint32_t sliceIndex, const void* data, VkDeviceSize size);

private:
  std::unique_ptr<DescriptorPool> descriptorPool;
  std::unordered_map<LayoutKey, std::unique_ptr<DescriptorSetLayout>>
      descriptorSetLayouts;

  // DescriptorSet Cache
  struct DescriptorKey {
    LayoutKey key;
    uint32_t frameIndex;

    bool operator==(const DescriptorKey &other) const {
      return key == other.key && frameIndex == other.frameIndex;
    }
  };
  struct DescriptorKeyHash {
    size_t operator()(const DescriptorKey &k) const {
      return (size_t)k.key*73856093u ^ k.frameIndex*19349663u;
    }
  };

  std::unordered_map<DescriptorKey, VkDescriptorSet, DescriptorKeyHash> setCache;

  PerFrameCamera cameraPF;

  void createDescriptorPool();
  void createDescriptorSetLayouts();
  void createPerFrameBuffers(uint32_t sliceCount);
  void writeDescriptorSet(LayoutKey key, uint32_t frameIndex);
};

}
