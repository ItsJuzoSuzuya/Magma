module;
#include <cassert>
#include <vulkan/vulkan_core.h>

module core:buffer;

namespace Magma {

export class Buffer {
public:
  Buffer(VkDeviceSize instanceSize, uint32_t instanceCount,
                 VkBufferUsageFlags usageFlags,
                 VkMemoryPropertyFlags memoryPropertyFlags,
                 VkDeviceSize minOffsetAlignment) {
    alignmentSize = getAlignment(instanceSize, minOffsetAlignment);
    bufferSize = alignmentSize * instanceCount;
    Device::get().createBuffer(bufferSize, usageFlags, memoryPropertyFlags,
                               buffer, bufferMemory);
  }

  ~Buffer() { cleanUp(); }
  void cleanUp() {
    if (buffer == VK_NULL_HANDLE && bufferMemory == VK_NULL_HANDLE)
      return;

    unmap();

    VkBuffer buf = buffer;
    VkDeviceMemory mem = bufferMemory;
    buffer = VK_NULL_HANDLE;
    bufferMemory = VK_NULL_HANDLE;

    DeletionQueue::push([buf, mem](VkDevice device) {
      vkDestroyBuffer(device, buf, nullptr);
      vkFreeMemory(device, mem, nullptr);
    });
  }

  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  // Getters
  VkBuffer &getBuffer() { return buffer; }
  VkDeviceSize getBufferSize() const { return bufferSize; }
  void *mappedData() const { return mappedMemory; }

  VkDescriptorBufferInfo descriptorInfo() {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.offset = 0;
    bufferInfo.range = bufferSize;
    bufferInfo.buffer = buffer;
    return bufferInfo;
  }

  // Memory Operations
  VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) {
    assert(buffer && bufferMemory && "Buffer has to be created before mapping!");

    VkDevice device = Device::get().device();
    return vkMapMemory(device, bufferMemory, offset, size, 0, &mappedMemory);
  }

  void writeToBuffer(void *data, VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) {
    assert(mappedMemory && "Cannot write to unmapped memory!");

    if (size == VK_WHOLE_SIZE) {
      memcpy(mappedMemory, data, bufferSize);
    } else {
      char *memOffset = (char *)mappedMemory;
      memOffset += offset;
      memcpy(memOffset, data, size);
    }
  }

  void flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) {
    VkDevice device = Device::get().device();
    const VkDeviceSize atom = Device::nonCoherentAtomSize();

    VkMappedMemoryRange mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = bufferMemory;

    if (size == VK_WHOLE_SIZE) {
      mappedRange.size = VK_WHOLE_SIZE;
      mappedRange.offset = 0;
    } else {
      VkDeviceSize alignedOffset = (offset / atom) * atom;
      VkDeviceSize end = offset + size;
      VkDeviceSize alignedEnd = ((end + atom - 1) / atom) * atom;
      if (alignedEnd > bufferSize)
        alignedEnd = bufferSize;
      VkDeviceSize alignedSize = alignedEnd - alignedOffset;

      mappedRange.offset = alignedOffset;
      mappedRange.size = alignedSize;
    }

    vkFlushMappedMemoryRanges(device, 1, &mappedRange);
  }

private:
  // Buffer
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
  void *mappedMemory = nullptr;

  // Buffer properties
  VkDeviceSize bufferSize;
  VkDeviceSize alignmentSize;
  VkDeviceSize getAlignment(VkDeviceSize instanceSize,
                                    VkDeviceSize minOffsetAlignment) {
    if (minOffsetAlignment == 0)
      return instanceSize;

    return (instanceSize + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
  }

  // Memory Operations
  void unmap() {
    if (mappedMemory) {
      VkDevice device = Device::get().device();
      vkUnmapMemory(device, bufferMemory);
      mappedMemory = nullptr;
    };
  }
};
} // namespace Magma
