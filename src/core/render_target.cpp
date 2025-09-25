#include "render_target.hpp"
#include "device.hpp"
#include "swapchain.hpp"

namespace Magma {

// Constructor

RenderTarget::RenderTarget(Device &device, SwapChain &swapChain)
    : device{device}, swapChain{swapChain} {}

// Images

void RenderTarget::createImages(uint32_t imagecount) {
  vkGetSwapchainImagesKHR(device.device(), swapChain, &imagecount, nullptr);
  swapChainImages.resize(imagecount);
  vkGetSwapchainImagesKHR(device.device(), swapChain, &imagecount,
                          swapChainImages.data());
}

void RenderTarget::createImageViews() {
  swapChainImageViews.resize(swapChainImages.size());
  for (size_t i = 0; i < swapChainImages.size(); i++) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = swapChainImages[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapChainImageFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device.device(), &viewInfo, nullptr,
                          &swapChainImageViews[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create image views!");
  }
}

// Depth Resources
void RenderTarget::createDepthResources() {
  depthImages.resize(swapChainImages.size());
  depthImageMemories.resize(swapChainImages.size());
  depthImageViews.resize(swapChainImages.size());

  for (size_t i = 0; i < swapChainImages.size(); i++) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = swapChainExtent.width;
    imageInfo.extent.height = swapChainExtent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = swapChainDepthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    device.createImageWithInfo(imageInfo, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
                               depthImages[i], depthImageMemories[i]);

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = depthImages[i];
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = swapChainDepthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device.device(), &viewInfo, nullptr,
                          &depthImageViews[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create image view!");
  }
}

// Framebuffers
void RenderTarget::createFramebuffers() {
  framebuffers.resize(swapChainImages.size());
  for (size_t i = 0; i < swapChainImages.size(); i++) {
    std::array<VkImageView, 2> attachments = {swapChainImageViews[i],
                                              depthImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr,
                            &framebuffers[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create framebuffers!");
  }
}

} // namespace Magma
