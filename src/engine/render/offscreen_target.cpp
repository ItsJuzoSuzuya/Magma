#include "offscreen_target.hpp"
#include "../core/device.hpp"
#include <array>
#include <print>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

// Constructor
OffscreenTarget::OffscreenTarget(const RenderTargetInfo &info)
    : targetExtent{info.extent}, imageFormat{info.colorFormat},
      depthImageFormat{info.depthFormat}, imageCount_{info.imageCount} {
  createImages();
  createIdImage();
  createDepthResources();
  createColorSampler();
}

OffscreenTarget::~OffscreenTarget() { cleanup(); }

void OffscreenTarget::cleanup() {
  VkDevice device = Device::get().device();

  if (colorSampler != VK_NULL_HANDLE) {
    vkDestroySampler(device, colorSampler, nullptr);
    colorSampler = VK_NULL_HANDLE;
  }

  destroyIdImages();
  destroyDepthResources();

  for (auto v : imageViews) {
    if (v != VK_NULL_HANDLE)
      vkDestroyImageView(device, v, nullptr);
  }
  imageViews.clear();

  destroyColorResources();
}

// Resize (Offscreen)
void OffscreenTarget::resize(VkExtent2D newExtent) {
  if (newExtent.width == 0 || newExtent.height == 0)
    return;
  if (newExtent.width == targetExtent.width &&
      newExtent.height == targetExtent.height)
    return;

  Device::waitIdle();
  cleanup();

  targetExtent = newExtent;

  createImages();
  createIdImage();
  createDepthResources();
  createColorSampler();
}

// --- Private helpers ---

// Images (color) - Offscreen
void OffscreenTarget::createImages() {
  images.resize(imageCount_);
  imageMemories.resize(imageCount_);

  VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = targetExtent.width;
  imageInfo.extent.height = targetExtent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = imageFormat;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  for (uint32_t i = 0; i < imageCount_; ++i)
    Device::get().createImageWithInfo(imageInfo,
                                      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                      images[i], imageMemories[i]);

  createImageViews();
}

void OffscreenTarget::createImageViews() {
  VkDevice device = Device::get().device();

  imageViews.resize(images.size());
  for (size_t i = 0; i < images.size(); ++i) {
    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = images[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = imageFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &viewInfo, nullptr, &imageViews[i]) !=
        VK_SUCCESS)
      throw std::runtime_error(
          "OffscreenRenderTarget: failed to create color image view");
  }
}

void OffscreenTarget::createDepthResources() {
  Device &device = Device::get();

  depthImages.resize(images.size());
  depthImageMemories.resize(images.size());
  depthImageViews.resize(images.size());

  for (size_t i = 0; i < images.size(); ++i) {
    VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = targetExtent.width;
    imageInfo.extent.height = targetExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthImageFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    device.createImageWithInfo(imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               depthImages[i], depthImageMemories[i]);

    VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    viewInfo.image = depthImages[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthImageFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device.device(), &viewInfo, nullptr,
                          &depthImageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error(
          "OffscreenRenderTarget: failed to create depth view");
    }
  }
}

void OffscreenTarget::createIdImage() {
  VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = targetExtent.width;
  imageInfo.extent.height = targetExtent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = idImageFormat;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  Device::get().createImageWithInfo(
      imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, idImage, idImageMemory);

  VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.image = idImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = idImageFormat;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(Device::get().device(), &viewInfo, nullptr,
                        &idImageView) != VK_SUCCESS)
    throw runtime_error(
        "OffscreenRenderTarget: failed to create id image view");
}

void OffscreenTarget::createColorSampler() {
  if (colorSampler != VK_NULL_HANDLE)
    return;

  VkSamplerCreateInfo info{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
  info.magFilter = VK_FILTER_LINEAR;
  info.minFilter = VK_FILTER_LINEAR;
  info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
  info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  info.minLod = 0.0f;
  info.maxLod = 0.0f;
  info.maxAnisotropy = 1.0f;
  info.anisotropyEnable = VK_FALSE;
  info.compareEnable = VK_FALSE;
  info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  info.unnormalizedCoordinates = VK_FALSE;

  VkDevice device = Device::get().device();
  if (vkCreateSampler(device, &info, nullptr, &colorSampler) != VK_SUCCESS)
    throw std::runtime_error("OffscreenRenderTarget: failed to create sampler");
}

// Destruction helpers
void OffscreenTarget::destroyIdImages() {
  VkDevice device = Device::get().device();
  if (idImageView != VK_NULL_HANDLE) {
    vkDestroyImageView(device, idImageView, nullptr);
    idImageView = VK_NULL_HANDLE;
  }
  if (idImage != VK_NULL_HANDLE) {
    vkDestroyImage(device, idImage, nullptr);
    idImage = VK_NULL_HANDLE;
  }
  if (idImageMemory != VK_NULL_HANDLE) {
    vkFreeMemory(device, idImageMemory, nullptr);
    idImageMemory = VK_NULL_HANDLE;
  }
}

void OffscreenTarget::destroyColorResources() {
  VkDevice device = Device::get().device();
  for (size_t i = 0; i < images.size(); ++i) {
    if (images[i] != VK_NULL_HANDLE)
      vkDestroyImage(device, images[i], nullptr);
    if (imageMemories[i] != VK_NULL_HANDLE)
      vkFreeMemory(device, imageMemories[i], nullptr);
  }
  images.clear();
  imageMemories.clear();
}

void OffscreenTarget::destroyDepthResources() {
  VkDevice device = Device::get().device();
  for (size_t i = 0; i < depthImages.size(); ++i) {
    if (depthImageViews[i] != VK_NULL_HANDLE)
      vkDestroyImageView(device, depthImageViews[i], nullptr);
    if (depthImages[i] != VK_NULL_HANDLE)
      vkDestroyImage(device, depthImages[i], nullptr);
    if (depthImageMemories[i] != VK_NULL_HANDLE)
      vkFreeMemory(device, depthImageMemories[i], nullptr);
  }
  depthImages.clear();
  depthImageViews.clear();
  depthImageMemories.clear();
}

} // namespace Magma
