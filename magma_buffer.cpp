#include "magma_buffer.hpp"
#include "magma_device.hpp"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace magma {

MagmaBuffer::MagmaBuffer(MagmaDevice &device, VkDeviceSize instanceSize,
                         uint32_t instanceCount, VkBufferUsageFlags usageFlags,
                         VkMemoryPropertyFlags propertieFlags,
                         VkDeviceSize minAlignment)
    : magmaDevice(device), instanceSize(instanceSize),
      instanceCount(instanceCount), usageFlags(usageFlags),
      propertieFlags(propertieFlags) {

  alignmentSize = getAlignment(instanceSize, minAlignment);
  bufferSize = alignmentSize * instanceCount;
  device.createBuffer(bufferSize, usageFlags, propertieFlags, buffer,
                      bufferMemory);
}

MagmaBuffer::~MagmaBuffer() {
  unmap();
  vkDestroyBuffer(magmaDevice.device(), buffer, nullptr);
  vkFreeMemory(magmaDevice.device(), bufferMemory, nullptr);
}

VkResult MagmaBuffer::map(VkDeviceSize size, VkDeviceSize offset) {
  assert(buffer && bufferMemory &&
         "Cannot map memory before buffer has been created");
  return vkMapMemory(magmaDevice.device(), bufferMemory, offset, size, 0,
                     &mapped);
}

void MagmaBuffer::unmap() {
  if (mapped) {
    vkUnmapMemory(magmaDevice.device(), bufferMemory);
    mapped = nullptr;
  }
}

VkDeviceSize MagmaBuffer::getAlignment(VkDeviceSize instanceSize,
                                       VkDeviceSize minAlignment) {
  if (minAlignment > 0) {
    instanceSize = (instanceSize + minAlignment - 1) & ~(minAlignment - 1);
  }
  return instanceSize;
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

void MagmaBuffer::writeToIndex(void *data, int index) {
  writeToBuffer(data, instanceSize, index * alignmentSize);
}

VkResult MagmaBuffer::flushIndex(int index) {
  return flush(alignmentSize, index * alignmentSize);
}
} // namespace magma
