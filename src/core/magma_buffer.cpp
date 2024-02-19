/*
 * Encapsulates a vulkan buffer
 *
 * Initially based off VulkanBuffer by Sascha Willems -
 * https://github.com/SaschaWillems/Vulkan/blob/master/base/VulkanBuffer.h
 */

#include "magma_buffer.hpp"

// std
#include <cassert>
#include <cstring>

namespace magma {

VkDeviceSize MagmaBuffer::getAlignment(VkDeviceSize instanceSize,
                                       VkDeviceSize minOffsetAlignment) {
  if (minOffsetAlignment > 0) {
    return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
  }
  return instanceSize;
}

MagmaBuffer::MagmaBuffer(MagmaDevice &device, VkDeviceSize instanceSize,
                         uint32_t instanceCount, VkBufferUsageFlags usageFlags,
                         VkMemoryPropertyFlags memoryPropertyFlags,
                         VkDeviceSize minOffsetAlignment)
    : magmaDevice{device}, instanceSize{instanceSize},
      instanceCount{instanceCount}, usageFlags{usageFlags},
      memoryPropertyFlags{memoryPropertyFlags} {
  alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
  bufferSize = alignmentSize * instanceCount;
  device.createBuffer(bufferSize, usageFlags, memoryPropertyFlags, buffer,
                      bufferMemory);
}

MagmaBuffer::~MagmaBuffer() {
  unmap();
  vkDestroyBuffer(magmaDevice.device(), buffer, nullptr);
  vkFreeMemory(magmaDevice.device(), bufferMemory, nullptr);
}

VkResult MagmaBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
  assert(buffer && bufferMemory && "Called map on buffer before create");
  return vkMapMemory(magmaDevice.device(), bufferMemory, offset, size, 0,
                     &mapped);
}

void MagmaBuffer::unmap() {
  if (mapped) {
    vkUnmapMemory(magmaDevice.device(), bufferMemory);
    mapped = nullptr;
  }
}

void MagmaBuffer::writeToBuffer(void *data, VkDeviceSize size,
                                VkDeviceSize offset) {
  assert(mapped && "Cannot copy to unmapped buffer");

  if (size == VK_WHOLE_SIZE) {
    memcpy(mapped, data, bufferSize);
  } else {
    char *memOffset = (char *)mapped;
    memOffset += offset;
    memcpy(memOffset, data, size);
  }
}

VkResult MagmaBuffer::flush(VkDeviceSize size, VkDeviceSize offset) {
  VkMappedMemoryRange mappedRange = {};
  mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  mappedRange.memory = bufferMemory;
  mappedRange.offset = offset;
  mappedRange.size = size;
  return vkFlushMappedMemoryRanges(magmaDevice.device(), 1, &mappedRange);
}

VkResult MagmaBuffer::invalidate(VkDeviceSize size, VkDeviceSize offset) {
  VkMappedMemoryRange mappedRange = {};
  mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
  mappedRange.memory = bufferMemory;
  mappedRange.offset = offset;
  mappedRange.size = size;
  return vkInvalidateMappedMemoryRanges(magmaDevice.device(), 1, &mappedRange);
}

VkDescriptorBufferInfo MagmaBuffer::descriptorInfo(VkDeviceSize size,
                                                   VkDeviceSize offset) {
  return VkDescriptorBufferInfo{
      buffer,
      offset,
      size,
  };
}

void MagmaBuffer::writeToIndex(void *data, int index) {
  writeToBuffer(data, instanceSize, index * alignmentSize);
}

VkResult MagmaBuffer::flushIndex(int index) {
  return flush(alignmentSize, index * alignmentSize);
}

VkDescriptorBufferInfo MagmaBuffer::descriptorInfoForIndex(int index) {
  return descriptorInfo(alignmentSize, index * alignmentSize);
}

VkResult MagmaBuffer::invalidateIndex(int index) {
  return invalidate(alignmentSize, index * alignmentSize);
}

} // namespace magma
