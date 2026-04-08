module;
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
#include <string.h>
#include <stdexcept>

export module features:object_picker;
import core;
import :render_feature;

namespace Magma {

class GameObject;

export class ObjectPicker: public RenderFeature {
public:
ObjectPicker(VkExtent2D extent, uint32_t imageCount): targetExtent{extent}, imageCount_{imageCount} {
  createImages();
  idImageLayouts.resize(imageCount_, VK_IMAGE_LAYOUT_UNDEFINED);
}
~ObjectPicker() { destroyImages(); }

void onResize(VkExtent2D newExtent) {
  if (newExtent.width == 0 || newExtent.height == 0)
    return;
  if (newExtent.width == targetExtent.width &&
      newExtent.height == targetExtent.height)
    return;

  destroyImages();

  targetExtent = newExtent;

  createImages();
  idImageLayouts.resize(imageCount_, VK_IMAGE_LAYOUT_UNDEFINED);
}

void prepare(uint32_t imageIndex) {
    // Transition ID image to COLOR_ATTACHMENT_OPTIMAL
    VkImageLayout idLayout = getIdImageLayout(imageIndex);
    if (idLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
      transitionIdImage(
          imageIndex, ImageTransition::ShaderReadToColorOptimal);
    else if (idLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) 
      transitionIdImage(
          imageIndex, ImageTransition::UndefinedToColorOptimal);
}

void pushColorAttachments(
    std::vector<VkRenderingAttachmentInfo> &colors,
    uint32_t imageIndex) {
  VkRenderingAttachmentInfo idAttachment = getIdAttachment(imageIndex);
  colors.emplace_back(idAttachment);
}

void finish(uint32_t imageIndex) {
  transitionIdImage(
      imageIndex, ImageTransition::ColorOptimalToShaderRead);

  servicePendingPick();
}

VkImage getIdImage(uint32_t imageIndex) const {
  return idImages[imageIndex]; }
VkImageView getIdImageView(uint32_t imageIndex) const {
  return idImageViews[imageIndex]; }
VkImageLayout getIdImageLayout(uint32_t imageIndex) const {
  return idImageLayouts[imageIndex]; }
VkRenderingAttachmentInfo getIdAttachment(uint32_t imageIndex) const {
  VkClearValue clearValue;
  clearValue.color = {};
  clearValue.color.uint32[0] = 0;
  clearValue.color.uint32[1] = 0;
  clearValue.color.uint32[2] = 0;
  clearValue.color.uint32[3] = 0;

  VkRenderingAttachmentInfo idAttachmentInfo{};
  idAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
  idAttachmentInfo.imageView = idImageViews[imageIndex];
  idAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  idAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  idAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  idAttachmentInfo.clearValue = clearValue;
  return idAttachmentInfo;
}

void transitionIdImage(size_t index,
                         ImageTransitionDescription transition) {
  VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.oldLayout = idImageLayouts.at(index);
  barrier.newLayout = transition.newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = idImages.at(index);
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = transition.srcAccess;
  barrier.dstAccessMask = transition.dstAccess;

  vkCmdPipelineBarrier(FrameInfo::commandBuffer, transition.srcStage,
                       transition.dstStage, 0, 0, nullptr, 0, nullptr, 1,
                       &barrier);
  idImageLayouts.at(index) = transition.newLayout;
}

void requestPick(uint32_t x, uint32_t y) {
  pendingPick.hasRequest = true;
  pendingPick.x = x;
  pendingPick.y = y;
  // result will be fetched later
}

void servicePendingPick() {
  if (!pendingPick.hasRequest)
    return;

  uint32_t picked = pickAtPixel(pendingPick.x, pendingPick.y);
  pendingPick.result = picked;
  pendingPick.hasRequest = false;
}

uint32_t pollPickResult() {
  uint32_t result = pendingPick.result;
  pendingPick.result = 0; 
  return result;
} // executes after offscreen rendering

private:
  uint32_t imageCount_ = 0;
  VkExtent2D targetExtent{};

  // Id image for object picking
  std::vector<VkImage> idImages;
  std::vector<VkDeviceMemory> idImageMemories;
  std::vector<VkImageView> idImageViews;
  std::vector<VkImageLayout> idImageLayouts;
  VkFormat idImageFormat = VK_FORMAT_R32_UINT;
  void createImages() {
    idImages.resize(imageCount_);
    idImageMemories.resize(imageCount_);
    idImageViews.resize(imageCount_);

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

    for (uint32_t i = 0; i < imageCount_; ++i) {
      Device::get().createImageWithInfo(
          imageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, idImages[i], idImageMemories[i]);

      VkImageViewCreateInfo viewInfo{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
      viewInfo.image = idImages[i];
      viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
      viewInfo.format = idImageFormat;
      viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      viewInfo.subresourceRange.baseMipLevel = 0;
      viewInfo.subresourceRange.levelCount = 1;
      viewInfo.subresourceRange.baseArrayLayer = 0;
      viewInfo.subresourceRange.layerCount = 1;

      if (vkCreateImageView(Device::get().device(), &viewInfo, nullptr,
                            &idImageViews[i]) != VK_SUCCESS)
        throw std::runtime_error(
            "OffscreenRenderTarget: failed to create id image view");
    }
  }

  void destroyImages() {
    VkDevice device = Device::get().device();
    for (size_t i = 0; i < idImages.size(); ++i) {
      if (idImageViews[i] != VK_NULL_HANDLE) {
        vkDestroyImageView(device, idImageViews[i], nullptr);
        idImageViews[i] = VK_NULL_HANDLE;
      }

      if (idImages[i] != VK_NULL_HANDLE) {
        vkDestroyImage(device, idImages[i], nullptr);
        idImages[i] = VK_NULL_HANDLE;
      }
      if (idImageMemories[i] != VK_NULL_HANDLE) {
        vkFreeMemory(device, idImageMemories[i], nullptr);
        idImageMemories[i] = VK_NULL_HANDLE;
      }
    }
    idImages.clear();
    idImageMemories.clear();
    idImageViews.clear();
  }

  // Deferred Picking
  struct PendingPick {
    bool hasRequest = false;
    uint32_t x = 0, y = 0;
    uint32_t result = 0;
  } pendingPick;

  uint32_t pickAtPixel(uint32_t x, uint32_t y) {
    Buffer stagingBuffer(sizeof(uint32_t), 1, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.map();

    VkImage idImage = idImages[FrameInfo::frameIndex];

    VkCommandBuffer cb = Device::get().beginSingleTimeCommands();

    // Transition ID image for readback (from shader read-only to transfer src)
    Device::transitionImageLayoutCmd(
        cb, idImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {static_cast<int32_t>(x), static_cast<int32_t>(y), 0};
    region.imageExtent = {1, 1, 1};

    Device::get().copyImageToBuffer(cb, stagingBuffer.getBuffer(),
                                    idImage, region);

    // Transition back to shader read-only
    Device::transitionImageLayoutCmd(
          cb, idImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
          VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
          VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
          VK_IMAGE_ASPECT_COLOR_BIT);

    Device::get().endSingleTimeCommands(cb);

    uint32_t objectId = 0;
    void *data = stagingBuffer.mappedData();
    if (data)
      memcpy(&objectId, data, sizeof(uint32_t));

    return objectId;
  }
};
}
