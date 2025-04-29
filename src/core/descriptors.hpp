#include "device.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace magma {

class DescriptorSetLayout {
public:
  class Builder {
  public:
    Builder(Device &device) : device{device} {};
    Builder &addBinding(uint32_t binding, VkDescriptorType descriptorType,
                        VkShaderStageFlags stageFlags, uint32_t count = 1);
    std::unique_ptr<DescriptorSetLayout> build() const;

  private:
    Device &device;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
  };

  DescriptorSetLayout(
      Device &device,
      std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
  ~DescriptorSetLayout();

  VkDescriptorSetLayout getDescriptorSetLayout() const {
    return descriptorSetLayout;
  }

private:
  Device &device;
  VkDescriptorSetLayout descriptorSetLayout;
  std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};

  friend class DescriptorWriter;
};

class DescriptorPool {
public:
  class Builder {
  public:
    Builder(Device &device) : device{device} {};

    Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
    Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
    Builder &setMaxSets(uint32_t count);
    std::unique_ptr<DescriptorPool> build() const;

  private:
    Device &device;
    std::vector<VkDescriptorPoolSize> poolSizes{};
    VkDescriptorPoolCreateFlags poolFlags{};
    uint32_t maxSets;
  };

  DescriptorPool(Device &device, uint32_t maxSets,
                 VkDescriptorPoolCreateFlags poolFlags,
                 const std::vector<VkDescriptorPoolSize> &poolSizes);
  ~DescriptorPool();

  bool allocateDescriptor(VkDescriptorSetLayout setLayout,
                          VkDescriptorSet &set);

private:
  Device &device;
  VkDescriptorPool descriptorPool = {};

  friend class DescriptorWriter;
};

class DescriptorWriter {
public:
  DescriptorWriter(DescriptorSetLayout &setLayout, DescriptorPool &pool)
      : setLayout{setLayout}, pool{pool} {};
  DescriptorWriter &writeBuffer(
      uint32_t binding, VkDescriptorBufferInfo *bufferInfo,
      VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
      VkDescriptorImageInfo *imageInfo = nullptr);
  DescriptorWriter &writeImage(uint32_t binding,
                               VkDescriptorImageInfo *imageInfo);

  bool build(VkDescriptorSet &set);
  void overwrite(VkDescriptorSet &set);

private:
  DescriptorSetLayout &setLayout;
  DescriptorPool &pool;
  std::vector<VkWriteDescriptorSet> writes;
};

} // namespace magma
