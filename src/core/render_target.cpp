#include "render_target.hpp"
#include "device.hpp"
#include <array>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

OffscreenRenderTarget::OffscreenRenderTarget(Device &device, VkExtent2D extent,
                                             VkFormat colorFormat,
                                             VkFormat depthFormat,
                                             uint32_t imageCount)
    : device{device}, targetExtent{extent}, imageFormat{colorFormat},
      depthImageFormat{depthFormat}, imageCount_{imageCount} {
  createImages();
  createImageViews();
  createRenderPass();
  createDepthResources();
  createFramebuffers();
  createColorSampler();
}

OffscreenRenderTarget::~OffscreenRenderTarget() {
  destroyFramebuffers();
  destroyDepthResources();
  destroyRenderPass();

  for (auto v : imageViews) {
    if (v != VK_NULL_HANDLE)
      vkDestroyImageView(device.device(), v, nullptr);
  }
  destroyColorResources();

  if (colorSampler != VK_NULL_HANDLE) {
    vkDestroySampler(device.device(), colorSampler, nullptr);
    colorSampler = VK_NULL_HANDLE;
  }
}

// Rendering helpers
void OffscreenRenderTarget::begin(VkCommandBuffer commandBuffer,
                                  uint32_t frameIndex) {
  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {{0.f, 0.f, 0.f, 1.f}};
  clearValues[1].depthStencil = {1.0f, 0};

  VkRenderPassBeginInfo beginInfo{VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
  beginInfo.renderPass = renderPass;
  beginInfo.framebuffer = framebuffers[frameIndex];
  beginInfo.renderArea.offset = {0, 0};
  beginInfo.renderArea.extent = targetExtent;
  beginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  beginInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {};
  viewport.x = 0;
  viewport.y = static_cast<float>(targetExtent.height);
  viewport.width = static_cast<float>(targetExtent.width);
  viewport.height = -static_cast<float>(targetExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent = targetExtent;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void OffscreenRenderTarget::end(VkCommandBuffer cmd) {
  vkCmdEndRenderPass(cmd);
}

void OffscreenRenderTarget::resize(VkExtent2D newExtent) {
  if (newExtent.width == 0 || newExtent.height == 0)
    return;
  if (newExtent.width == targetExtent.width &&
      newExtent.height == targetExtent.height)
    return;

  vkDeviceWaitIdle(device.device());

  destroyFramebuffers();
  destroyDepthResources();
  destroyRenderPass();

  for (auto v : imageViews) {
    if (v != VK_NULL_HANDLE)
      vkDestroyImageView(device.device(), v, nullptr);
  }
  destroyColorResources();

  targetExtent = newExtent;

  createImages();
  createImageViews();
  createRenderPass();
  createDepthResources();
  createFramebuffers();
  createColorSampler();
}

// Private

void OffscreenRenderTarget::createImages() {
  images.resize(imageCount_);
  imageMemories.resize(imageCount_);

  VkImageCreateInfo imageInfo{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = targetExtent.width;
  imageInfo.extent.height = targetExtent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = imageFormat; // mirror swapchain color format
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                    VK_IMAGE_USAGE_SAMPLED_BIT |
                    VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  for (uint32_t i = 0; i < imageCount_; ++i) {
    // Keep this flag consistent with your Device::createImageWithInfo
    // expectations
    device.createImageWithInfo(imageInfo, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
                               images[i], imageMemories[i]);
  }
}

void OffscreenRenderTarget::createImageViews() {
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

    if (vkCreateImageView(device.device(), &viewInfo, nullptr,
                          &imageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error(
          "OffscreenRenderTarget: failed to create color image view");
    }
  }
}

void OffscreenRenderTarget::createRenderPass() {
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = imageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  // We will sample this in ImGui
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  VkAttachmentReference colorRef{};
  colorRef.attachment = 0;
  colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = depthImageFormat;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthRef{};
  depthRef.attachment = 1;
  depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorRef;
  subpass.pDepthStencilAttachment = &depthRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  std::array<VkAttachmentDescription, 2> attachments{colorAttachment,
                                                     depthAttachment};

  VkRenderPassCreateInfo rpInfo{VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
  rpInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  rpInfo.pAttachments = attachments.data();
  rpInfo.subpassCount = 1;
  rpInfo.pSubpasses = &subpass;
  rpInfo.dependencyCount = 1;
  rpInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device.device(), &rpInfo, nullptr, &renderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error(
        "OffscreenRenderTarget: failed to create render pass");
  }
}

void OffscreenRenderTarget::createDepthResources() {
  depthImages.resize(images.size());
  depthImageMemories.resize(images.size());
  depthImageViews.resize(images.size());
  depthImageLayouts.resize(images.size(),
                           VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

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

    device.createImageWithInfo(imageInfo, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
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

void OffscreenRenderTarget::createFramebuffers() {
  framebuffers.resize(images.size());
  for (size_t i = 0; i < images.size(); ++i) {
    std::array<VkImageView, 2> atts{imageViews[i], depthImageViews[i]};

    VkFramebufferCreateInfo fbInfo{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    fbInfo.renderPass = renderPass;
    fbInfo.attachmentCount = static_cast<uint32_t>(atts.size());
    fbInfo.pAttachments = atts.data();
    fbInfo.width = targetExtent.width;
    fbInfo.height = targetExtent.height;
    fbInfo.layers = 1;

    if (vkCreateFramebuffer(device.device(), &fbInfo, nullptr,
                            &framebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error(
          "OffscreenRenderTarget: failed to create framebuffer");
    }
  }
}

void OffscreenRenderTarget::createColorSampler() {
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

  if (vkCreateSampler(device.device(), &info, nullptr, &colorSampler) !=
      VK_SUCCESS) {
    throw std::runtime_error("OffscreenRenderTarget: failed to create sampler");
  }
}

void OffscreenRenderTarget::destroyColorResources() {
  for (size_t i = 0; i < images.size(); ++i) {
    if (images[i] != VK_NULL_HANDLE) {
      vkDestroyImage(device.device(), images[i], nullptr);
    }
    if (imageMemories[i] != VK_NULL_HANDLE) {
      vkFreeMemory(device.device(), imageMemories[i], nullptr);
    }
  }
  images.clear();
  imageMemories.clear();
  imageViews.clear();
}

void OffscreenRenderTarget::destroyDepthResources() {
  for (size_t i = 0; i < depthImages.size(); ++i) {
    if (depthImageViews[i] != VK_NULL_HANDLE) {
      vkDestroyImageView(device.device(), depthImageViews[i], nullptr);
    }
    if (depthImages[i] != VK_NULL_HANDLE) {
      vkDestroyImage(device.device(), depthImages[i], nullptr);
    }
    if (depthImageMemories[i] != VK_NULL_HANDLE) {
      vkFreeMemory(device.device(), depthImageMemories[i], nullptr);
    }
  }
  depthImages.clear();
  depthImageViews.clear();
  depthImageMemories.clear();
  depthImageLayouts.clear();
}

void OffscreenRenderTarget::destroyFramebuffers() {
  for (auto fb : framebuffers) {
    if (fb != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device.device(), fb, nullptr);
    }
  }
  framebuffers.clear();
}

void OffscreenRenderTarget::destroyRenderPass() {
  if (renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device.device(), renderPass, nullptr);
    renderPass = VK_NULL_HANDLE;
  }
}

VkFormat OffscreenRenderTarget::findDepthFormat() {
  return device.findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

} // namespace Magma
