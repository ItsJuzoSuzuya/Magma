#pragma once
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace Magma {

class DescriptorSetLayout {
public:
  class Builder {
  public:
    Builder &addBinding(uint32_t binding, VkDescriptorType descriptorType,
                        VkShaderStageFlags stageFlags, uint32_t count = 1);
    std::unique_ptr<DescriptorSetLayout> build() const;

  private:
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
  };

  DescriptorSetLayout(
      std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
  ~DescriptorSetLayout();

  VkDescriptorSetLayout getDescriptorSetLayout() const {
    return descriptorSetLayout;
  }

private:
  VkDescriptorSetLayout descriptorSetLayout;
  std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};

  friend class DescriptorWriter;
};

class DescriptorPool {
public:
  class Builder {
  public:
    Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
    Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
    Builder &setMaxSets(uint32_t count);
    std::unique_ptr<DescriptorPool> build() const;

  private:
    std::vector<VkDescriptorPoolSize> poolSizes{};
    VkDescriptorPoolCreateFlags poolFlags{};
    uint32_t maxSets;
  };

  DescriptorPool(uint32_t maxSets,
                 VkDescriptorPoolCreateFlags poolFlags,
                 const std::vector<VkDescriptorPoolSize> &poolSizes);
  ~DescriptorPool();

  bool allocateDescriptor(VkDescriptorSetLayout setLayout,
                          VkDescriptorSet &set);
  VkDescriptorPool getDescriptorPool() const {
    if (descriptorPool == VK_NULL_HANDLE)
      throw std::runtime_error("Descriptor Pool is not created!");
    return descriptorPool;
  }

private:
  VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

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

} // namespace Magma
