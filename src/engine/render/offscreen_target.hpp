#pragma once
#include "core/render_target.hpp"
#include "core/render_target_info.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>
#include "engine/render/features/object_picker.hpp"


namespace Magma {

class GameObject;

class OffscreenTarget : public IRenderTarget {
public:
  explicit OffscreenTarget(const RenderTargetInfo &info);
  ~OffscreenTarget();

  VkExtent2D extent() const override { return targetExtent; }
  uint32_t imageCount() const override { return imageCount_; }

  VkImage getColorImage(size_t index) override;
  VkImageView getColorImageView(size_t index) const override;
  VkRenderingAttachmentInfo getColorAttachment(size_t index) const override;
  uint32_t getColorAttachmentCount() const override { return 2; }
  void transitionColorImage(size_t index, ImageTransitionDescription transition) override;
  VkImageLayout getColorImageLayout(size_t index) const override;
  VkFormat getColorFormat() const override { return imageFormat; }

  VkImageView getDepthImageView(size_t index) const;
  VkRenderingAttachmentInfo getDepthAttachment(size_t index) const override;
  VkFormat getDepthFormat() const override { return depthImageFormat; }

  VkSampler getColorSampler() const override { return colorSampler; }

  void onResize(VkExtent2D newExtent) override;
  void cleanup() override;

private:
  // Color images (offscreen-owned)
  uint32_t imageCount_ = 0;
  std::vector<VkImage> images;
  std::vector<VkImageView> imageViews;
  std::vector<VkDeviceMemory> imageMemories;
  std::vector<VkImageLayout> imageLayouts;
  VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
  void createImages();
  void createImageViews();
  void destroyColorResources();

  // Depth images (offscreen-owned)
  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemories;
  std::vector<VkImageView> depthImageViews;
  std::vector<VkImageLayout> depthImageLayouts;
  VkFormat depthImageFormat = VK_FORMAT_D32_SFLOAT;
  void createDepthResources();
  void destroyDepthResources();

  // Sampler for sampling the color image in shaders / ImGui
  VkSampler colorSampler{VK_NULL_HANDLE};
  void createColorSampler();

  // Extent
  VkExtent2D targetExtent{};

};
} // namespace Magma
