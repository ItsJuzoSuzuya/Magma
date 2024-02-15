#pragma once

#include "magma_device.hpp"
#include <cstdint>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace magma {

class MagmaBuffer {
public:
  MagmaBuffer(MagmaDevice &device, VkDeviceSize instanceSize,
              uint32_t instanceCount, VkBufferUsageFlags usageFlags,
              VkMemoryPropertyFlags propertieFlags,
              VkDeviceSize minAlignment = 1);
  ~MagmaBuffer();

  MagmaBuffer(const MagmaBuffer &) = delete;
  MagmaBuffer &operator=(const MagmaBuffer &) = delete;

  void writeToBuffer(void *data, VkDeviceSize size = VK_WHOLE_SIZE,
                     VkDeviceSize offset = 0);
  VkResult flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

  void writeToIndex(void *data, int index);
  VkResult flushIndex(int index);

  VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void unmap();

  VkBuffer getBuffer() { return buffer; }

private:
  static VkDeviceSize getAlignment(VkDeviceSize instanceSize,
                                   VkDeviceSize minAlignment);

  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory bufferMemory = VK_NULL_HANDLE;

  MagmaDevice &magmaDevice;
  void *mapped = nullptr;

  VkDeviceSize bufferSize;
  VkDeviceSize alignmentSize;
  VkDeviceSize instanceSize;

  uint32_t instanceCount;

  VkBufferUsageFlags usageFlags;
  VkMemoryPropertyFlags propertieFlags;
};

} // namespace magma
