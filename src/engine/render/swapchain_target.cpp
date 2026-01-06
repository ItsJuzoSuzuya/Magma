#include "swapchain_target.hpp"
#include "core/device.hpp"
#include "core/render_target_info.hpp"
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

SwapchainTarget::SwapchainTarget(VkExtent2D extent, RenderTargetInfo &info) {
  swapChain_ = std::make_unique<SwapChain>(extent);
  info = swapChain_->getRenderInfo();

  targetExtent = info.extent;
  imageFormat = info.colorFormat;
  depthImageFormat = info.depthFormat;
  imageCount_ = info.imageCount;

  createImages();
  createImageViews();
  createDepthResources();
}

SwapchainTarget::~SwapchainTarget() { cleanup(); }

// ----------------------------------------------------------------------------
// Public Methods
// ----------------------------------------------------------------------------

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

// Color resources
VkImage SwapchainTarget::getColorImage(size_t index) {
  return images[index];
}

VkImageView SwapchainTarget::getColorImageView(size_t index) const {
  return imageViews[index];
}

VkRenderingAttachmentInfo SwapchainTarget::getColorAttachment(
    size_t index) const {
  VkRenderingAttachmentInfo colorAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
  colorAttachment.imageView = imageViews[index];
  colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.clearValue.color = {{0.0f, 0.0f, 0.0f, 1.0f}};
  return colorAttachment;
}

VkImageLayout SwapchainTarget::getColorImageLayout(size_t index) const {
  return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
}

void SwapchainTarget::transitionColorImage(size_t index,
                                           ImageTransitionDescription transition) {
  Device::transitionImageLayout(
      images[index], imageLayouts[index],
      transition.newLayout, transition.srcAccess,
      transition.dstAccess, transition.srcStage,
      transition.dstStage);
  imageLayouts[index] = transition.newLayout;
}


// Depth resources
VkImageView SwapchainTarget::getDepthImageView(size_t index) const {
  return depthImageViews[index];
}

VkRenderingAttachmentInfo SwapchainTarget::getDepthAttachment(
    size_t index) const {
  VkRenderingAttachmentInfo depthAttachment{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
  depthAttachment.imageView = depthImageViews[index];
  depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.clearValue.depthStencil = {1.0f, 0};
  return depthAttachment;
}

void SwapchainTarget::onResize(const VkExtent2D newExtent) {
  if (newExtent.width == 0 || newExtent.height == 0)
    return;

  Device::waitIdle();
  cleanup();

  // Recreate swapchain
  std::shared_ptr<SwapChain> oldSwapChain = std::move(swapChain_);
  swapChain_ = std::make_unique<SwapChain>(newExtent, oldSwapChain);

  createImages();
  createImageViews();
  createDepthResources();

  targetExtent = newExtent;
}

// ----------------------------------------------------------------------------
// Private Methods
// ----------------------------------------------------------------------------

void SwapchainTarget::createImages() {
  VkDevice device = Device::get().device();
  vkGetSwapchainImagesKHR(device, swapChain_->getSwapChain(), &imageCount_, nullptr);
  images.resize(imageCount_);
  vkGetSwapchainImagesKHR(device, swapChain_->getSwapChain(), &imageCount_, images.data());
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
