module;
#include <cstdint>
#include <vulkan/vulkan_core.h>

module core:frame_info;

namespace Magma {

export struct FrameInfo {
  inline static int frameIndex = 0;
  inline static uint32_t imageIndex = 0;
  inline static VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
  inline static VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

  static void advanceFrame() {
    frameIndex = (frameIndex + 1) % SwapChain::MAX_FRAMES_IN_FLIGHT;
  }
};

} // namespace Magma
