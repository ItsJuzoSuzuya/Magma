#pragma once
#include "device.hpp"
#include "render_target_info.hpp"
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

class SwapChain;

enum class RenderType { Offscreen, Swapchain };

class RenderTarget {
public:
  // Constructors / Destructor
  RenderTarget(Device &device, RenderTargetInfo info);
  RenderTarget(Device &device, SwapChain &swapChain);
  ~RenderTarget();
  void cleanup();

  RenderTarget(const RenderTarget &) = delete;
  RenderTarget &operator=(const RenderTarget &) = delete;

  // Getters
  VkRenderPass getRenderPass() { return renderPass; }
  VkFramebuffer getFrameBuffer(int index) { return framebuffers[index]; }
  VkImage &getDepthImage(int index) { return depthImages[index]; }
  VkExtent2D &extent() { return targetExtent; }
  float extentAspectRatio() {
    return static_cast<float>(targetExtent.width) /
           static_cast<float>(targetExtent.height);
  }

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
  void resize(VkExtent2D newExtent, VkSwapchainKHR swapChain);

private:
  // Type of render target
  RenderType type;

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
  void createImages(VkSwapchainKHR swapChain);
  void createImageViews();

  // Render Pass
  VkRenderPass renderPass = VK_NULL_HANDLE;
  void createRenderPass(VkImageLayout finalLayout);

  // Depth
  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemories;
  std::vector<VkImageView> depthImageViews;
  VkFormat depthImageFormat;
  void createDepthResources();

  // Framebuffers
  std::vector<VkFramebuffer> framebuffers;
  void createFramebuffers();

  // Color Sampler (for sampling the offscreen color images)
  VkSampler colorSampler{VK_NULL_HANDLE};
  void createColorSampler();
};
} // namespace Magma
