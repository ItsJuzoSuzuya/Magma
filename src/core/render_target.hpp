#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>
namespace Magma {

class Device;
class SwapChain;

class RenderTarget {
public:
  RenderTarget(Device &device);
  void create(VkExtent2D extent);

  ~RenderTarget();
  void destroy();

private:
  Device &device;
  SwapChain &swapChain;
  VkExtent2D windowExtent;

  // Images
  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  void createImages(uint32_t imagecount);
  void createImageViews();

  // Depth Resources
  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemories;
  std::vector<VkImageView> depthImageViews;
  std::vector<VkImageLayout> depthImageLayouts;
  VkFormat depthFormat;
  void createDepthResources();

  // Framebuffers
  std::vector<VkFramebuffer> framebuffers;
  void createFramebuffers();
};

} // namespace Magma
