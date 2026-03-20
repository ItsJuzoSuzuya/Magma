module;
#include <cstdint>
#include <vulkan/vulkan_core.h>
module core:render_target_info;

namespace Magma {

struct RenderTargetInfo {
  VkExtent2D extent;
  VkFormat colorFormat;
  VkFormat depthFormat;
  uint32_t imageCount;
};

} // namespace Magma
