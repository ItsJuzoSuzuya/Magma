#pragma once
#include "core/buffer.hpp"
#include "core/descriptors.hpp"
#include "core/swapchain.hpp"
#include <array>
#include <memory>
#include <unordered_map>
#include <vulkan/vulkan_core.h>

namespace Magma {

enum class LayoutKey {
  PointLight = 0,
};

/**
 * Singleton that owns scene-global GPU resources shared across all renderers.
 * Currently manages the point light SSBO and its descriptor sets.
 */
class RenderContext {
public:
  RenderContext() = default;
  ~RenderContext();

  RenderContext(const RenderContext &) = delete;
  RenderContext &operator=(const RenderContext &) = delete;

  VkDescriptorSetLayout getLayout(LayoutKey key);
  VkDescriptorSet getDescriptorSet(LayoutKey key, uint32_t frameIndex);

  void updatePointLights(uint32_t frameIndex, const void *data, VkDeviceSize size);

private:
  std::unique_ptr<DescriptorPool> descriptorPool;
  void ensureDescriptorPool();

  std::unordered_map<LayoutKey, std::unique_ptr<DescriptorSetLayout>> layouts;
  void ensureLayout(LayoutKey key);

  std::array<std::unique_ptr<Buffer>, SwapChain::MAX_FRAMES_IN_FLIGHT> pointLightBuffers;
  std::array<VkDescriptorSet, SwapChain::MAX_FRAMES_IN_FLIGHT> pointLightSets{};
  bool pointLightInitialized = false;
  void initPointLightBuffers();
};

} // namespace Magma
