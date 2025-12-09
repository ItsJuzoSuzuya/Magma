#include "swapchain_target.hpp"
#include "../core/device.hpp"
#include <array>
#include <print>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

SwapchainTarget::SwapchainTarget(SwapChain &swapChain) {
  auto info = swapChain.getRenderInfo();
  targetExtent = info.extent;
  imageFormat = info.colorFormat;
  depthImageFormat = info.depthFormat;
  imageCount_ = info.imageCount;

  createImages(swapChain.getSwapChain());
  createImageViews();
  createDepthResources();
}

SwapchainTarget::~SwapchainTarget() { cleanup(); }

void SwapchainTarget::cleanup() {
  VkDevice device = Device::get().device();

  destroyDepthResources();

  for (auto v : imageViews) {
    if (v != VK_NULL_HANDLE)
      vkDestroyImageView(device, v, nullptr);
  }

  // Note: swapchain images themselves are owned by the swapchain and must not
  // be destroyed here.
}

// Resize (Swapchain)
bool SwapchainTarget::resize(VkExtent2D newExtent, VkSwapchainKHR swapChain) {
  if (newExtent.width == 0 || newExtent.height == 0)
    return false;

  Device::waitIdle();

  cleanup();
  targetExtent = newExtent;

  createImages(swapChain);
  createImageViews();
  createDepthResources();
  return true;
}

// --- Private helpers ---

void SwapchainTarget::createImages(VkSwapchainKHR swapChain) {
  VkDevice device = Device::get().device();
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount_, nullptr);
  images.resize(imageCount_);
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount_, images.data());
}

void SwapchainTarget::createImageViews() {
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
          "SwapchainTarget: failed to create color image view");
  }
}

void SwapchainTarget::createDepthResources() {
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
      throw std::runtime_error("SwapchainTarget: failed to create depth view");
    }
  }
}

// Destruction helpers
void SwapchainTarget::destroyDepthResources() {
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
