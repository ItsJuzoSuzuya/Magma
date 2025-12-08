#pragma once
#include "../core/render_target.hpp"
#include "../core/render_target_info.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

class GameObject;

class OffscreenTarget : public RenderTarget {
public:
  explicit OffscreenTarget(const RenderTargetInfo &info);
  ~OffscreenTarget() override;

  // RenderTarget interface
  VkExtent2D extent() const override { return targetExtent; }

  VkImage &getColorImage(int index) override {
    return images.at(static_cast<size_t>(index));
  }
  VkImageView getColorImageView(int index) const override {
    return imageViews.at(static_cast<size_t>(index));
  }
  VkSampler getColorSampler() const override { return colorSampler; }
  VkFormat getColorFormat() const override { return imageFormat; }
  uint32_t getColorAttachmentCount() const override { return 2; }

  VkFormat getDepthFormat() const override { return depthImageFormat; }
  VkImageView getDepthImageView(int index) const {
    return depthImageViews.at(static_cast<size_t>(index));
  }

  VkImage &getIdImage() { return idImage; }
  VkImageView getIdImageView() const { return idImageView; }
  uint32_t imageCount() const override { return imageCount_; }

  void resize(VkExtent2D newExtent) override;
  void cleanup() override;

private:
  // Color images (offscreen-owned)
  uint32_t imageCount_ = 0;
  std::vector<VkImage> images;
  std::vector<VkImageView> imageViews;
  std::vector<VkDeviceMemory> imageMemories;
  VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
  void createImages();
  void createImageViews();

  // Depth images (offscreen-owned)
  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemories;
  std::vector<VkImageView> depthImageViews;
  VkFormat depthImageFormat = VK_FORMAT_D32_SFLOAT;
  void createDepthResources();

  // Id image for object picking
  VkImage idImage = VK_NULL_HANDLE;
  VkDeviceMemory idImageMemory = VK_NULL_HANDLE;
  VkImageView idImageView = VK_NULL_HANDLE;
  VkFormat idImageFormat = VK_FORMAT_R32_UINT;
  void createIdImage();

  // Sampler for sampling the color image in shaders / ImGui
  VkSampler colorSampler{VK_NULL_HANDLE};
  void createColorSampler();

  // Extent
  VkExtent2D targetExtent{};

  // Destruction helpers
  void destroyIdImages();
  void destroyColorResources();
  void destroyDepthResources();
};
} // namespace Magma
