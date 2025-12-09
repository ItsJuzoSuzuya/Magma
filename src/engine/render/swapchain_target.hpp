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
  VkExtent2D extent() const override { return targetExtent; }

  // Swapchain-backed targets do not own color images; they expose them.
  VkImage &getColorImage(int index) override {
    return images.at(static_cast<size_t>(index));
  }
  VkImageView getColorImageView(int index) const override {
    return imageViews.at(static_cast<size_t>(index));
  }
  VkSampler getColorSampler() const override {
    return VK_NULL_HANDLE;
  } // no sampler for swapchain
  VkFormat getColorFormat() const override { return imageFormat; }
  VkFormat getDepthFormat() const override { return depthImageFormat; }
  VkImageView getDepthImageView(int index) const {
    return depthImageViews.at(static_cast<size_t>(index));
  }

  // Resize signature for swapchain
  bool resize(VkExtent2D newExtent, VkSwapchainKHR swapChain) override;
  void cleanup() override;

  uint32_t imageCount() const override { return imageCount_; }
  uint32_t getColorAttachmentCount() const override { return 1; }

private:
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

  // Extent
  VkExtent2D targetExtent{};

  // Internal helpers (adapted)
  void createImages(VkSwapchainKHR swapChain);
  void createImageViews();
  void createDepthResources();

  // Destruction helpers
  void destroyDepthResources();
};
} // namespace Magma
