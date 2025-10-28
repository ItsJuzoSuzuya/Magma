#pragma once
#include "../core/render_target.hpp"
#include "../core/swapchain.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

class SwapchainTarget : public RenderTarget {
public:
  explicit SwapchainTarget(SwapChain &swapChain);
  ~SwapchainTarget() override;

  // RenderTarget interface
  VkRenderPass getRenderPass() const override { return renderPass; }
  VkFramebuffer getFrameBuffer(int index) const override {
    return framebuffers.at(static_cast<size_t>(index));
  }
  VkExtent2D extent() const override { return targetExtent; }

  // Swapchain-backed targets do not own color images; they expose them.
  VkImage &getColorImage(int index) override { return images.at(static_cast<size_t>(index)); }
  VkImageView getColorImageView(int index) const override { return imageViews.at(static_cast<size_t>(index)); }
  VkSampler getColorSampler() const override { return VK_NULL_HANDLE; } // no sampler for swapchain
  VkFormat getColorFormat() const override { return imageFormat; }
  VkFormat getDepthFormat() const override { return depthImageFormat; }

  // Resize signature for swapchain
  void resize(VkExtent2D newExtent, VkSwapchainKHR swapChain) override;
  void cleanup() override;

private:
  // Render pass
  VkRenderPass renderPass = VK_NULL_HANDLE;

  // Swapchain images (owned by swapchain) - we keep views
  uint32_t imageCount_ = 0;
  std::vector<VkImage> images;
  std::vector<VkImageView> imageViews;
  VkFormat imageFormat = VK_FORMAT_B8G8R8A8_SRGB;

  // Depth (owned)
  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemories;
  std::vector<VkImageView> depthImageViews;
  VkFormat depthImageFormat = VK_FORMAT_D32_SFLOAT;

  // Framebuffers
  std::vector<VkFramebuffer> framebuffers;

  // Extent
  VkExtent2D targetExtent{};

  // Internal helpers (adapted)
  void createImages(VkSwapchainKHR swapChain);
  void createImageViews();
  void createRenderPass(VkImageLayout finalLayout);
  void createDepthResources();
  void createFramebuffers();

  // Destruction helpers
  void destroyDepthResources();
  void destroyFramebuffers();
  void destroyRenderPass();
};
} // namespace Magma
