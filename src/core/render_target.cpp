#include "render_target.hpp"
#include "device.hpp"
#include "render_target_info.hpp"
#include "swapchain.hpp"
#include <array>
#include <cassert>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

// Offscreen Render Target
RenderTarget::RenderTarget(Device &device, RenderTargetInfo info)
    : device{device}, targetExtent{info.extent}, imageFormat{info.colorFormat},
      depthImageFormat{info.depthFormat}, imageCount_{info.imageCount} {
  type = RenderType::Offscreen;

  createImages();
  createImageViews();
  createRenderPass(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  createDepthResources();
  createFramebuffers();
  createColorSampler();
}

// Swapchain Render Target
RenderTarget::RenderTarget(Device &device, SwapChain &swapChain)
    : device{device} {
  type = RenderType::Swapchain;

  auto info = swapChain.getRenderInfo();
  targetExtent = info.extent;
  imageFormat = info.colorFormat;
  depthImageFormat = info.depthFormat;
  imageCount_ = info.imageCount;

  createImages(swapChain.getSwapChain());
  createImageViews();
  createRenderPass(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  createDepthResources();
  createFramebuffers();
}

// Destructor
RenderTarget::~RenderTarget() { cleanup(); }

void RenderTarget::cleanup() {
  destroyFramebuffers();
  destroyDepthResources();
  destroyRenderPass();

  if (type == RenderType::Offscreen) {
    for (auto v : imageViews) {
      if (v != VK_NULL_HANDLE)
        vkDestroyImageView(device.device(), v, nullptr);
    }
    destroyColorResources();
    printf("Destroyed Color Resources\n");
  }

  if (colorSampler != VK_NULL_HANDLE) {
    vkDestroySampler(device.device(), colorSampler, nullptr);
    colorSampler = VK_NULL_HANDLE;
    printf("Destroyed Color Sampler\n");
  }
}

//--- Public ---
// Resize (Offscreen)
void RenderTarget::resize(VkExtent2D newExtent) {
  assert(
      type == RenderType::Offscreen &&
      "RenderTarget::resize(extent) can only be called on Offscreen targets");

  if (newExtent.width == 0 || newExtent.height == 0)
    return;
  if (newExtent.width == targetExtent.width &&
      newExtent.height == targetExtent.height)
    return;

  vkDeviceWaitIdle(device.device());
  cleanup();

  targetExtent = newExtent;

  createImages();
  createImageViews();
  createRenderPass(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  createDepthResources();
  createFramebuffers();
  createColorSampler();
}

// Resize (Swapchain)
void RenderTarget::resize(VkExtent2D newExtent, VkSwapchainKHR swapChain) {
  assert(type == RenderType::Swapchain &&
         "RenderTarget::resize(extent, swapChain) can only be called on "
         "Swapchain ");

  if (newExtent.width == 0 || newExtent.height == 0)
    return;
  if (newExtent.width == targetExtent.width &&
      newExtent.height == targetExtent.height)
    return;

  vkDeviceWaitIdle(device.device());
  cleanup();

  targetExtent = newExtent;

  createImages(swapChain);
  createImageViews();
  createRenderPass(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
  createDepthResources();
  createFramebuffers();
  createColorSampler();
}

//--- Private ---

// Images (color)
//  -> Offscreen
void RenderTarget::createImages() {
  assert(
      type == RenderType::Offscreen &&
      "RenderTarget::createImages() can only be called on Offscreen targets");
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

  for (uint32_t i = 0; i < imageCount_; ++i) {
    // Keep this flag consistent with your Device::createImageWithInfo
    // expectations
    device.createImageWithInfo(imageInfo, VK_MEMORY_HEAP_DEVICE_LOCAL_BIT,
                               images[i], imageMemories[i]);
  }
}

//  -> Swapchain
void RenderTarget::createImages(VkSwapchainKHR swapChain) {
  assert(type == RenderType::Swapchain &&
         "RenderTarget::createImages(swapChain) can only be called on "
         "Swapchain targets");
  vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount_, nullptr);
  images.resize(imageCount_);
  vkGetSwapchainImagesKHR(device.device(), swapChain, &imageCount_,
                          images.data());
}

// Image Views (color)
void RenderTarget::createImageViews() {
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
          "RenderTarget: failed to create color image view");
    }
  }
}

// Render Pass
void RenderTarget::createRenderPass(VkImageLayout finalLayout) {
  assert(
      (type == RenderType::Offscreen &&
       finalLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) ||
      (type == RenderType::Swapchain &&
       finalLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) &&
          "RenderTarget::createRenderPass(finalLayout) - finalLayout must be "
          "VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL for Offscreen targets "
          "and VK_IMAGE_LAYOUT_PRESENT_SRC_KHR for Swapchain targets");

  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = imageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = finalLayout;

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
    throw std::runtime_error("RenderTarget: failed to create render pass");
  }
}

// Depth Resources
void RenderTarget::createDepthResources() {
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
      throw std::runtime_error("RenderTarget: failed to create depth view");
    }
  }
}

// Framebuffers
void RenderTarget::createFramebuffers() {
  framebuffers.resize(images.size());
  for (size_t i = 0; i < images.size(); ++i) {
    std::array<VkImageView, 2> attachments{imageViews[i], depthImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = targetExtent.width;
    framebufferInfo.height = targetExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device.device(), &framebufferInfo, nullptr,
                            &framebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("RenderTarget: failed to create framebuffer");
    }
  }
}

// Sampler (for using color image as a texture)
void RenderTarget::createColorSampler() {
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
    throw std::runtime_error("RenderTarget: failed to create sampler");
  }
}

// Destruction helpers
void RenderTarget::destroyColorResources() {
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

void RenderTarget::destroyDepthResources() {
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
}

void RenderTarget::destroyFramebuffers() {
  for (auto fb : framebuffers) {
    if (fb != VK_NULL_HANDLE) {
      vkDestroyFramebuffer(device.device(), fb, nullptr);
    }
  }
  framebuffers.clear();
}

void RenderTarget::destroyRenderPass() {
  if (renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device.device(), renderPass, nullptr);
    renderPass = VK_NULL_HANDLE;
  }
}

} // namespace Magma
