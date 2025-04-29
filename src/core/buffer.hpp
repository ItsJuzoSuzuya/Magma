#ifndef BUFFER_HPP
#define BUFFER_HPP
#include "device.hpp"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace magma {

class Buffer {
public:
  Buffer(Device &device, VkDeviceSize instanceSize, uint32_t instanceCount,
         VkBufferUsageFlags usageFlags,
         VkMemoryPropertyFlags memoryPropertyFlags,
         VkDeviceSize minOffsetAlignment = 0);
  ~Buffer();
  void cleanUp();

  Buffer(const Buffer &) = delete;
  Buffer &operator=(const Buffer &) = delete;

  VkBuffer &getBuffer() { return buffer; }
  VkDeviceSize getBufferSize() const { return bufferSize; }
  void *mappedData() const { return mappedMemory; }

  VkDescriptorBufferInfo descriptorInfo();

  VkResult map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
  void writeToBuffer(void *data, VkDeviceSize size = VK_WHOLE_SIZE,
                     VkDeviceSize offset = 0);
  void getDepthBufferData(VkCommandBuffer &commandBuffer, const VkImage &image,
                          const VkExtent2D &extent,
                          std::vector<float> &depthData,
                          VkDeviceSize offset = 0);
  void flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);

private:
  VkBuffer buffer = VK_NULL_HANDLE;
  VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
  void *mappedMemory = nullptr;

  Device &device;

  VkDeviceSize alignmentSize;
  VkDeviceSize bufferSize;

  VkDeviceSize getAlignment(VkDeviceSize instanceSize,
                            VkDeviceSize minOffsetAlignment);
  void unmap();
};
} // namespace magma
#endif
