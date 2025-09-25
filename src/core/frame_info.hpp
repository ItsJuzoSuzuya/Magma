#pragma once
#include <vulkan/vulkan_core.h>

namespace Magma {

struct FrameInfo {
  int frameIndex;
  VkCommandBuffer commandBuffer;
  VkDescriptorSet &descriptorSet;
};

} // namespace Magma
