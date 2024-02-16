#include "magma_camera.hpp"
#include <vulkan/vulkan_core.h>
namespace magma {
struct FrameInfo {
  int frameIndex;
  float frameTime;
  VkCommandBuffer commandBuffer;
  MagmaCamera &camera;
  VkDescriptorSet globalDescriptorSet;
};

} // namespace magma
