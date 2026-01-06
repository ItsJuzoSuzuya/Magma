#pragma once
#include "core/image_transitions.hpp"
#include "core/render_target.hpp"
#include "core/swapchain.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

class SwapchainTarget : public IRenderTarget {
public:
  explicit SwapchainTarget(VkExtent2D extent, RenderTargetInfo &info);
  ~SwapchainTarget() override;

  uint32_t imageCount() const override { return imageCount_; }
  VkExtent2D extent() const override { return targetExtent; }
  SwapChain* swapChain() const { return swapChain_.get(); }

  VkImage getColorImage(size_t index) override;
  VkImageView getColorImageView(size_t index) const  override;
  VkRenderingAttachmentInfo getColorAttachment(size_t index) const override;
  uint32_t getColorAttachmentCount() const override { return 1; }
  VkImageLayout getColorImageLayout(size_t index) const override;
  void transitionColorImage(size_t index, ImageTransitionDescription transition) override;
  VkFormat getColorFormat() const override { return imageFormat; }

  VkImageView getDepthImageView(size_t index) const;
  VkRenderingAttachmentInfo getDepthAttachment(size_t index) const override;
  VkFormat getDepthFormat() const override { return depthImageFormat; }

  VkSampler getColorSampler() const override { return VK_NULL_HANDLE; } // no sampler for swapchain

  void onResize(const VkExtent2D newExtent) override;
  void cleanup() override;

private:
  std::unique_ptr<SwapChain> swapChain_ = nullptr;
  VkExtent2D targetExtent = {0, 0};

  // Swapchain images (owned by swapchain) - we keep views
  uint32_t imageCount_ = 0;
  std::vector<VkImage> images = {VK_NULL_HANDLE};
  std::vector<VkImageView> imageViews = {VK_NULL_HANDLE};
  std::vector<VkImageLayout> imageLayouts = {VK_IMAGE_LAYOUT_UNDEFINED};
  VkFormat imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
  void createImages();
  void createImageViews();

  // Depth (owned)
  std::vector<VkImage> depthImages = {VK_NULL_HANDLE};
  std::vector<VkDeviceMemory> depthImageMemories = {VK_NULL_HANDLE};
  std::vector<VkImageView> depthImageViews = {VK_NULL_HANDLE};
  std::vector<VkImageLayout> depthImageLayouts = {VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
  VkFormat depthImageFormat = VK_FORMAT_D32_SFLOAT;
  void createDepthResources();
  void destroyDepthResources();








};
} // namespace Magma
