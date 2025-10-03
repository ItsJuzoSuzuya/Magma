#pragma once

#include <vulkan/vulkan_core.h>
namespace Magma {

struct RenderTargetInfo {
  VkExtent2D extent;
  VkFormat colorFormat;
  VkFormat depthFormat;
  uint32_t imageCount;
};

} // namespace Magma
