#pragma once
#include "../core/render_target.hpp"
#include "../core/render_target_info.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

class OffscreenTarget : public RenderTarget {
public:
  explicit OffscreenTarget(const RenderTargetInfo &info);
  ~OffscreenTarget() override;

  // RenderTarget interface
  VkRenderPass getRenderPass() const override { return renderPass; }
  VkFramebuffer getFrameBuffer(int index) const override {
    return framebuffers.at(static_cast<size_t>(index));
  }
  VkExtent2D extent() const override { return targetExtent; }

  VkImage &getColorImage(int index) override { return images.at(static_cast<size_t>(index)); }
  VkImageView getColorImageView(int index) const override {
    return imageViews.at(static_cast<size_t>(index));
  }
  VkSampler getColorSampler() const override { return colorSampler; }
  VkFormat getColorFormat() const override { return imageFormat; }
  VkFormat getDepthFormat() const override { return depthImageFormat; }

  void resize(VkExtent2D newExtent) override;
  void cleanup() override;

private:
  // Rendering resources
  VkRenderPass renderPass = VK_NULL_HANDLE;

  // Color images (offscreen-owned)
  uint32_t imageCount_ = 0;
  std::vector<VkImage> images;
  std::vector<VkImageView> imageViews;
  std::vector<VkDeviceMemory> imageMemories;
  VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

  // Depth images (offscreen-owned)
  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemories;
  std::vector<VkImageView> depthImageViews;
  VkFormat depthImageFormat = VK_FORMAT_D32_SFLOAT;

  // Framebuffers
  std::vector<VkFramebuffer> framebuffers;

  // Sampler for sampling the color image in shaders / ImGui
  VkSampler colorSampler{VK_NULL_HANDLE};

  // Extent
  VkExtent2D targetExtent{};

  // Internal helpers (copied/adapted from original implementation)
  void createImages();
  void createImageViews();
  void createRenderPass(VkImageLayout finalLayout);
  void createDepthResources();
  void createFramebuffers();
  void createColorSampler();

  // Destruction helpers
  void destroyColorResources();
  void destroyDepthResources();
  void destroyFramebuffers();
  void destroyRenderPass();
};
} // namespace Magma
