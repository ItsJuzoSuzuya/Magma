#pragma once

#include "magma_device.hpp"

// std
#include <memory>
#include <unordered_map>
#include <vector>

namespace magma {

class MagmaDescriptorSetLayout {
public:
  class Builder {
  public:
    Builder(MagmaDevice &magmaDevice) : magmaDevice{magmaDevice} {}

    Builder &addBinding(uint32_t binding, VkDescriptorType descriptorType,
                        VkShaderStageFlags stageFlags, uint32_t count = 1);
    std::unique_ptr<MagmaDescriptorSetLayout> build() const;

  private:
    MagmaDevice &magmaDevice;
    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings{};
  };

  MagmaDescriptorSetLayout(
      MagmaDevice &magmaDevice,
      std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings);
  ~MagmaDescriptorSetLayout();
  MagmaDescriptorSetLayout(const MagmaDescriptorSetLayout &) = delete;
  MagmaDescriptorSetLayout &
  operator=(const MagmaDescriptorSetLayout &) = delete;

  VkDescriptorSetLayout getDescriptorSetLayout() const {
    return descriptorSetLayout;
  }

private:
  MagmaDevice &magmaDevice;
  VkDescriptorSetLayout descriptorSetLayout;
  std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings;

  friend class MagmaDescriptorWriter;
};

class MagmaDescriptorPool {
public:
  class Builder {
  public:
    Builder(MagmaDevice &magmaDevice) : magmaDevice{magmaDevice} {}

    Builder &addPoolSize(VkDescriptorType descriptorType, uint32_t count);
    Builder &setPoolFlags(VkDescriptorPoolCreateFlags flags);
    Builder &setMaxSets(uint32_t count);
    std::unique_ptr<MagmaDescriptorPool> build() const;

  private:
    MagmaDevice &magmaDevice;
    std::vector<VkDescriptorPoolSize> poolSizes{};
    uint32_t maxSets = 1000;
    VkDescriptorPoolCreateFlags poolFlags = 0;
  };

  MagmaDescriptorPool(MagmaDevice &magmaDevice, uint32_t maxSets,
                      VkDescriptorPoolCreateFlags poolFlags,
                      const std::vector<VkDescriptorPoolSize> &poolSizes);
  ~MagmaDescriptorPool();
  MagmaDescriptorPool(const MagmaDescriptorPool &) = delete;
  MagmaDescriptorPool &operator=(const MagmaDescriptorPool &) = delete;

  bool allocateDescriptor(const VkDescriptorSetLayout descriptorSetLayout,
                          VkDescriptorSet &descriptor) const;

  void freeDescriptors(std::vector<VkDescriptorSet> &descriptors) const;

  void resetPool();

private:
  MagmaDevice &magmaDevice;
  VkDescriptorPool descriptorPool;

  friend class MagmaDescriptorWriter;
};

class MagmaDescriptorWriter {
public:
  MagmaDescriptorWriter(MagmaDescriptorSetLayout &setLayout,
                        MagmaDescriptorPool &pool);

  MagmaDescriptorWriter &writeBuffer(uint32_t binding,
                                     VkDescriptorBufferInfo *bufferInfo);
  MagmaDescriptorWriter &writeImage(uint32_t binding,
                                    VkDescriptorImageInfo *imageInfo);

  bool build(VkDescriptorSet &set);
  void overwrite(VkDescriptorSet &set);

private:
  MagmaDescriptorSetLayout &setLayout;
  MagmaDescriptorPool &pool;
  std::vector<VkWriteDescriptorSet> writes;
};

} // namespace magma
