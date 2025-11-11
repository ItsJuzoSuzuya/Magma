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
  Device &device = Device::get();

  uint32_t targetWidth = extent.width / 4;
  uint32_t targetHeight = extent.height / 4;

  const size_t pixelCount = static_cast<size_t>(targetWidth) * targetHeight;
  const VkDeviceSize bufferSizeBytes = pixelCount * sizeof(float);

  // Ensure output vector has the right size
  depthData.resize(pixelCount);

  Buffer stagingBuffer(sizeof(float), static_cast<uint32_t>(pixelCount),
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  stagingBuffer.map();

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  vkResetCommandBuffer(commandBuffer, 0);
  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) 
    throw std::runtime_error("Failed to begin command buffer!");

  device.transitionDepthImage(commandBuffer, srcImage,
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  VkBufferImageCopy region = {};
  region.bufferOffset = offset;
  region.bufferRowLength = 0;   // tightly packed
  region.bufferImageHeight = 0; // tightly packed
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {targetWidth, targetHeight, 1};

  // Issue copy from image to buffer
  device.copyImageToBuffer(commandBuffer, stagingBuffer.getBuffer(), srcImage, region);

  // Transition depth image back to depth-stencil attachment layout
  device.transitionDepthImage(commandBuffer, srcImage,
                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                              VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

  // Submit and wait for completion (Device::submitCommands ends and submits the buffer)
  device.submitCommands(commandBuffer);

  // Read back from mapped staging buffer into depthData
  void *mapped = stagingBuffer.mappedData();
  if (mapped) {
    std::memcpy(depthData.data(), mapped, bufferSizeBytes);
  } else {
    throw runtime_error("Failed to map staging buffer for depth readback!");
  }
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
