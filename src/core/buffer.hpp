#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

class Buffer {
public:
  Buffer(VkDeviceSize instanceSize, uint32_t instanceCount,
         VkBufferUsageFlags usageFlags,
         VkMemoryPropertyFlags memoryPropertyFlags,
         VkDeviceSize minOffsetAlignment = 0);
  ~Buffer();
  void cleanUp();

  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  // Getters
  VkBuffer &getBuffer() { return buffer; }
  VkDeviceSize getBufferSize() const { return bufferSize; }
  void *mappedData() const { return mappedMemory; }

  // Descriptor Info
  VkDescriptorBufferInfo descriptorInfo();

  // Memory Operations
  VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void writeToBuffer(void *data, VkDeviceSize size = VK_WHOLE_SIZE,
                     VkDeviceSize offset = 0);
  void getDepthBufferData(VkCommandBuffer &commandBuffer, const VkImage &image,
                          const VkExtent2D &extent,
                          std::vector<float> &depthData,
                          VkDeviceSize offset = 0);
  void flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

private:
  // Buffer
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
  void *mappedMemory = nullptr;

  // Buffer properties 
  VkDeviceSize bufferSize;
  VkDeviceSize alignmentSize;
  VkDeviceSize getAlignment(VkDeviceSize instanceSize,
                            VkDeviceSize minOffsetAlignment);

  // Memory Operations
  void unmap();
};
} // namespace Magma
