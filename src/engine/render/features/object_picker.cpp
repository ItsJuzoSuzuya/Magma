
#include "engine/render/features/object_picker.hpp"
#include "core/buffer.hpp"
#include "core/device.hpp"
#include "core/frame_info.hpp"
#include "core/render_target_info.hpp"
#include "engine/scene.hpp"

namespace Magma {

ObjectPicker::ObjectPicker(const RenderTargetInfo &info): targetExtent{info.extent}, imageCount_{info.imageCount} {
  createImages();
  idImageLayouts.resize(imageCount_, VK_IMAGE_LAYOUT_UNDEFINED);
}

// -----------------------------------------------------------------------------
// Public Methods
// -----------------------------------------------------------------------------

VkRenderingAttachmentInfo ObjectPicker::getIdAttachment(uint32_t imageIndex) const {
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

void ObjectPicker::requestPick(uint32_t x, uint32_t y) {
  pendingPick.hasRequest = true;
  pendingPick.x = x;
  pendingPick.y = y;
  // result will be fetched later
}

GameObject *ObjectPicker::pollPickResult() {
  GameObject *result = pendingPick.result;
  pendingPick.result = nullptr; 
  return result;
}

// -----------------------------------------------------------------------------
// Private Methods
// -----------------------------------------------------------------------------

void ObjectPicker::createImages() {
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

void ObjectPicker::destroyImages() {
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

GameObject *ObjectPicker::pickAtPixel(uint32_t x, uint32_t y) {
    Buffer stagingBuffer(sizeof(uint32_t), 1, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    stagingBuffer.map();

    VkImage idImage = idImages[FrameInfo::imageIndex];

    // Transition ID image for readback (from shader read-only to transfer src)
    Device::transitionImageLayout(
        idImage, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
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

    Device::get().copyImageToBuffer(FrameInfo::commandBuffer, stagingBuffer.getBuffer(),
                                    idImage, region);

    // Transition back to shader read-only
    Device::transitionImageLayout(
        idImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT);

    uint32_t objectId = 0;
    void *data = stagingBuffer.mappedData();
    if (data)
      memcpy(&objectId, data, sizeof(uint32_t));

    if (objectId == 0) 
      return nullptr;

    if (Scene::current())
      return Scene::current()->findGameObjectById(
          static_cast<GameObject::id_t>(objectId));

    return nullptr;
  }

  void ObjectPicker::servicePendingPick() {
    if (!pendingPick.hasRequest)
      return;

    GameObject *picked = pickAtPixel(pendingPick.x, pendingPick.y);
    pendingPick.result = picked;
    pendingPick.hasRequest = false;
  }

} // namespace Magma
