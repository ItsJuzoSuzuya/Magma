#pragma once
#include <cwchar>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

class Device;

class OffscreenRenderTarget {
public:
  OffscreenRenderTarget(Device &device, VkExtent2D extent, VkFormat colorFormat,
                        VkFormat depthFormat, uint32_t imageCount);
  ~OffscreenRenderTarget();

  OffscreenRenderTarget(const OffscreenRenderTarget &) = delete;
  OffscreenRenderTarget &operator=(const OffscreenRenderTarget &) = delete;

  // Getters
  VkRenderPass getRenderPass() { return renderPass; }
  VkFramebuffer getFrameBuffer(int index) { return framebuffers[index]; }
  VkImage &getDepthImage(int index) { return depthImages[index]; }
  VkExtent2D &extent() { return targetExtent; }
  float extentAspectRatio() {
    return static_cast<float>(targetExtent.width) /
           static_cast<float>(targetExtent.height);
  }
  VkImageLayout &getImageLayout(int index) { return depthImageLayouts[index]; }

  // Added minimal getters to use this as a texture (e.g., for ImGui)
  VkImage &getColorImage(int index) { return images[index]; }
  VkImageView getColorImageView(int index) { return imageViews[index]; }
  VkSampler getColorSampler() { return colorSampler; }
  VkFormat getColorFormat() const { return imageFormat; }
  VkFormat getDepthFormat() const { return depthImageFormat; }

  // Rendering helpers
  void begin(VkCommandBuffer cmd, uint32_t frameIndex);
  void end(VkCommandBuffer cmd);

  // Resize (recreates everything but keeps formats and imageCount)
  void resize(VkExtent2D newExtent);

private:
  Device &device;
  VkExtent2D targetExtent{};

  // Destruction helpers
  void destroyColorResources();
  void destroyDepthResources();
  void destroyFramebuffers();
  void destroyRenderPass();

  // Images (color)
  uint32_t imageCount_;
  std::vector<VkImage> images;
  std::vector<VkImageView> imageViews;
  std::vector<VkDeviceMemory> imageMemories;
  VkFormat imageFormat;
  void createImages();
  void createImageViews();

  // Render Pass
  VkRenderPass renderPass{VK_NULL_HANDLE};
  void createRenderPass();

  // Depth
  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemories;
  std::vector<VkImageView> depthImageViews;
  std::vector<VkImageLayout> depthImageLayouts;
  VkFormat depthImageFormat;
  void createDepthResources();
  VkFormat findDepthFormat();

  // Framebuffers
  std::vector<VkFramebuffer> framebuffers;
  void createFramebuffers();

  // Color Sampler (for sampling the offscreen color images)
  VkSampler colorSampler{VK_NULL_HANDLE};
  void createColorSampler();
};
} // namespace Magma
