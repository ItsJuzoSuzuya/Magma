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

Buffer::Buffer(VkDeviceSize instanceSize, uint32_t instanceCount,
               VkBufferUsageFlags usageFlags,
               VkMemoryPropertyFlags memoryPropertyFlags,
               VkDeviceSize minOffsetAlignment) {
  alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
  bufferSize = alignmentSize * instanceCount;
  Device::get().createBuffer(bufferSize, usageFlags, memoryPropertyFlags,
                             buffer, bufferMemory);
}

// Destructor

Buffer::~Buffer() {
  Device::waitIdle();
  cleanUp();
}

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
  return vkMapMemory(device, bufferMemory, offset, size, 0, &mappedMemory);
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
