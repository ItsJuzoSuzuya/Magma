#include "buffer.hpp"
#include "device.hpp"
#include "render_system.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <glm/common.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>
using namespace std;
namespace Magma {

// Constructor

Buffer::Buffer(VkDeviceSize instanceSize,
               uint32_t instanceCount, VkBufferUsageFlags usageFlags,
               VkMemoryPropertyFlags memoryPropertyFlags,
               VkDeviceSize minOffsetAlignment) {
  alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
  bufferSize = alignmentSize * instanceCount;
  Device::get().createBuffer(bufferSize, usageFlags, memoryPropertyFlags, buffer,
                      bufferMemory);
}

// Destructor

Buffer::~Buffer() { cleanUp(); }
void Buffer::cleanUp() {
  unmap();

  VkDevice device = Device::get().device();
  vkDestroyBuffer(device, buffer, nullptr);
  vkFreeMemory(device, bufferMemory, nullptr);
}
// --- Public ---
// Descriptor Info

VkDescriptorBufferInfo Buffer::descriptorInfo() {
  VkDescriptorBufferInfo bufferInfo = {};
  bufferInfo.offset = 0;
  bufferInfo.range = bufferSize;
  bufferInfo.buffer = buffer;
  return bufferInfo;
}

// Memory Operations

VkResult Buffer::map(VkDeviceSize size, VkDeviceSize offset) {
  assert(buffer && bufferMemory && "Buffer has to be created before mapping!");

  VkDevice device = Device::get().device();
  return vkMapMemory(device, bufferMemory, offset, size, 0,
                     &mappedMemory);
}

void Buffer::writeToBuffer(void *data, VkDeviceSize size, VkDeviceSize offset) {
  assert(mappedMemory && "Cannot write to unmapped memory!");

  if (size == VK_WHOLE_SIZE) {
    memcpy(mappedMemory, data, bufferSize);
  } else {
    char *memOffset = (char *)mappedMemory;
    memOffset += offset;
    memcpy(memOffset, data, size);
  }
}

void Buffer::getDepthBufferData(VkCommandBuffer &commandBuffer,
                                const VkImage &srcImage,
                                const VkExtent2D &extent,
                                std::vector<float> &depthData,
                                VkDeviceSize offset) {
  assert(mappedMemory && "Cannot read from unmapped memory!");

  Device &device = Device::get();

  VkImageCreateInfo imageInfo = {};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.depth = 1.f;
  imageInfo.extent.width = extent.width / 4;
  imageInfo.extent.height = extent.height / 4;
  imageInfo.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  imageInfo.format = VK_FORMAT_D32_SFLOAT;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  VkImage dstDepthImage;
  VkDeviceMemory depthImageMemory;
  device.createImageWithInfo(imageInfo,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                             dstDepthImage, depthImageMemory);

  VkImageBlit blit = {};
  blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  blit.srcSubresource.mipLevel = 0;
  blit.srcSubresource.baseArrayLayer = 0;
  blit.srcSubresource.layerCount = 1;
  blit.srcOffsets[1] = {static_cast<int32_t>(extent.width),
                        static_cast<int32_t>(extent.height), 1};

  blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  blit.dstSubresource.mipLevel = 0;
  blit.dstSubresource.baseArrayLayer = 0;
  blit.dstSubresource.layerCount = 1;
  blit.dstOffsets[1] = {static_cast<int32_t>(extent.width / 4.f),
                        static_cast<int32_t>(extent.height / 4.f), 1};

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  vkResetCommandBuffer(commandBuffer, 0);
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  device.transitionDepthImage(commandBuffer, srcImage,
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  device.transitionDepthImage(commandBuffer, dstDepthImage,
                              VK_IMAGE_LAYOUT_UNDEFINED,
                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  vkCmdBlitImage(commandBuffer, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 dstDepthImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                 VK_FILTER_NEAREST);

  VkBufferImageCopy region = {};
  region.bufferOffset = offset;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {extent.width / 4, extent.height / 4, 1};

  device.copyImageToBuffer(commandBuffer, buffer, dstDepthImage, region);

  std::memcpy(depthData.data(), mappedMemory, depthData.size() * sizeof(float));

  device.transitionDepthImage(commandBuffer, srcImage,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  device.submitCommands(commandBuffer);

  vkDestroyImage(device.device(), dstDepthImage, nullptr);
  vkFreeMemory(device.device(), depthImageMemory, nullptr);
}

void Buffer::flush(VkDeviceSize size, VkDeviceSize offset) {
  VkDevice device = Device::get().device();

  VkMappedMemoryRange mappedRange = {};
  mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  mappedRange.size = size;
  mappedRange.offset = offset;
  mappedRange.memory = bufferMemory;
  vkFlushMappedMemoryRanges(device, 1, &mappedRange);
}

// --- Private ---
// Buffer properties

VkDeviceSize Buffer::getAlignment(VkDeviceSize instanceSize,
                                  VkDeviceSize minOffsetAlignment) {
  if (minOffsetAlignment == 0)
    return instanceSize;

  return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
}

// Memory Operations

void Buffer::unmap() {
  if (mappedMemory) {
    VkDevice device = Device::get().device();
    vkUnmapMemory(device, bufferMemory);
    mappedMemory = nullptr;
  };
}

} // namespace Magma
